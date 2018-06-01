/** @file
 * Implementacja klasy przechowującej przekierowania numerów telefonicznych
 *
 * @author Michał Chojnowski <mc394134@students.mimuw.edu.pl>
 * @copyright Michał Chojnowski
 * @date 18.05.2018
 */

#include <stdlib.h>
#include <string.h>
#include "phone_forward.h"

/**
 * Struktura przechowująca ciąg numerów telefonów.
 */
struct PhoneNumbers {
	size_t size; ///< Rozmiar tablicy numerów.
	const char *data[]; ///< Tablica numerów.
};

/**
 * @brief Wydajna mapa, której dziedziną są słowa.
 *
 * Każdemu wierzcholkowi drzewa odpowiada dokładnie jedno słowo.
 * Każdy wierzchołek posiada etykietę (label). Słowo, które odpowiada
 * wierzchołkowi, jest konkatenacją wszystkich etykiet na ścieżce od korzenia
 * do tego wierzchołka włącznie. Etykiety rodzeństwa nie mają wspólnych
 * prefiksów. Rodzeństwo jest posortowane leksykograficznie względem etykiet.
 *
 * Każdy wierzchołek może należeć do "cyklu przekierowań". W jednym cyklu
 * przekierowań znajduje się jedno słowo z drzewa "to" (patrz PhoneForward)
 * oraz wszystkie słowa z drzewa "from", które są na nie przekierowane.
 */
struct RadixTree {
	/** Wskazuje na lewego brata wierzchołka, jeśli istnieje,
	 * lub na rodzica, a w przypadku korzenia - na siebie. */
	struct RadixTree *leftSibling;

	/** Wskazuje na prawego brata wierzchołka, jeśli istnieje,
	 * lub na rodzica, a w przypadku korzenia - na siebie. */
	struct RadixTree *rightSibling;

	/** Wskazuje na skrajnie prawe (a więc pierwsze od lewej) dziecko,
	 * jeśli istnieje, lub na siebie. */
	struct RadixTree *leftChild;

	/** Wskazuje na skrajnie lewe (a więc pierwsze od prawej) dziecko,
	 * jeśli istnieje, lub na siebie. */
	struct RadixTree *rightChild;

	/** Wskazuje na lewego sąsiada w cyklu przekierowań, jeśli istnieje,
	 * lub na siebie. */
	struct RadixTree *leftRev;

	/** Wskazuje na prawego sąsiada w cyklu przekierowań, jeśli istnieje,
	 * lub na siebie. */
	struct RadixTree *rightRev;

	/** Etykieta wierzchołka. */
	char *label;

	/** Pełne słowo odpowiadające wierzchołkowi, jeśli znajduje się on
	 * w cyklu przekierowań, lub NULL. */
	char *fullWord;

	/** Odpowiedni wierzchołek z drzewa "to", na który dany wierzchołek
	 * z "from" jest przekierowany, lub NULL.*/
	struct RadixTree *fwd;

	unsigned charset;
	unsigned labelLength;
};

/**
 * Struktura przechowująca przekierowania numerów telefonów.
 */
struct PhoneForward {
	/** Drzewo słów, które są przekierowane. */
	struct RadixTree *from;

	/** Drzewo słów, na które istnieją przekierowania. */
	struct RadixTree *to;
};

/** Typedef dla zwięzłości. */
typedef struct RadixTree rt;


////////////////////////////////////////////////////////////////////////////////
// Pomocnicze operacje na stringach

/**
 * @brief Alokuje kopię stringu.
 *
 * @param begin Początek stringu.
 * @param end Wskaźnik za ostatni znak do skopiowania. Jeśli wynosi NULL, to
 * kopiowane jest cały string aż do '\0'.
 * @return Wskaźnik na kopię lub NULL w przypadku błędu alokacji.
 */
char *
copyString (const char *begin, const char *end)
{
	size_t len = end ? (size_t)(end - begin) : strlen(begin);
	char *new = malloc(len + 1);
	if (!new) return NULL;
	memcpy (new, begin, len);
	new[len] = '\0';
	return new;
}

/**
 * @brief Alokuje konkatenację dwóch stringów.
 *
 * @param prefix Pierwszy string.
 * @param suffix Drugi string.
 *
 * @return Wskaźnik na kopię lub NULL w przypadku błędu alokacji.
 */
char *
mergeStrings (const char *prefix, const char *suffix)
{
	char *new = malloc(strlen(prefix) + strlen(suffix) + 1);
	if (!new) return NULL;
	strcpy(new, prefix);
	strcat(new, suffix);
	return new;
}

/**
 * @brief Sprawdza czy string składa się w całości z cyfr.
 *
 * @param arg Dany string.
 */
bool
isNumber(const char *arg)
{
	if (!arg || !*arg)
		return false;
	for (; *arg; ++arg) {
		if (*arg < '0' || *arg > ';') return false;
	}
	return true;
}


////////////////////////////////////////////////////////////////////////////////
// Operacje na wierzcholkach

static unsigned charset (const char*);
void
setLabel(rt* arg, char *label)
{
	arg->label = label;
	arg->labelLength = label ? strlen(label) : 0;
	arg->charset = label ? charset(label) : 0;
}

/**
 * @brief Alokuje nowy wierzchołek drzewa słów.
 *
 * @return Wskaźnik na nowy wierzchołek lub NULL w przypadku błędu alokacji.
 */
rt *
makeRT()
{
	rt *new = malloc(sizeof(rt));
	if (!new) return NULL;
	*new = (rt){new, new, new, new, new, new, NULL, NULL, NULL, 0, 0};
	return new;
}

/**
 * @brief Zwraca właściwy wskaźnik wskazujący na dany wierzchołek z jego lewego
 * brata/rodzica.
 *
 * @param arg Dany wierzchołek.
 */
rt **
fromLeftSibling(rt *arg)
{
	return arg->leftSibling->rightSibling == arg ?
		&arg->leftSibling->rightSibling : &arg->leftSibling->rightChild;
}

/**
 * @brief Zwraca właściwy wskaźnik wskazujący na dany wierzchołek z jego prawego
 * brata/rodzica.
 *
 * @param arg Dany wierzchołek.
 */
rt **
fromRightSibling(rt *arg)
{
	return arg->rightSibling->leftSibling == arg ?
		&arg->rightSibling->leftSibling : &arg->rightSibling->leftChild;
}

/**
 * @brief Wstawia nowy wierzchołek pomiędzy wierzchołkiem danym a jego
 * rodzicem.
 *
 * @param arg Dany wierzchołek.
 * @param breakpoint Miejsce, w którym etykieta arg ma zostać podzielona
 * między arg i jego nowego rodzica.
 *
 * @return Nowy wierzchołek lub NULL w przypadku błędu alokacji.
 */
rt *
addAbove (rt *arg, const char* breakpoint)
{
	rt *new = makeRT();
	if (!new) return NULL;

	setLabel(new, copyString(arg->label, breakpoint));
	char *argLabel = copyString(breakpoint, NULL);
	if (!new->label || !argLabel) goto alloc_error;

	*fromLeftSibling(arg) = new;
	*fromRightSibling(arg) = new;
	new->leftSibling = arg->leftSibling;
	new->rightSibling = arg->rightSibling;
	new->leftChild = new->rightChild = arg;
	arg->leftSibling = arg->rightSibling = new;

	free(arg->label);
	setLabel(arg, argLabel);

	return new;

alloc_error:
	free(new->label);
	free(argLabel);
	free(new);
	return NULL;
}

/**
 * @brief Wstawia nowy wierzchołek jako lewego brata danego.
 *
 * @param arg Dany wierzchołek.
 * @param label Etykieta nowego wierzchołka.
 *
 * @return Nowy wierzchołek lub NULL w przypadku błędu alokacji.
 */
rt *
addLeft(rt *arg, const char *label)
{
	rt *new = makeRT();
	if (!new) return NULL;
	char *newLabel = copyString(label, NULL);
	if (!newLabel) {free(new); return NULL;};

	*fromLeftSibling(arg) = new;
	new->leftSibling = arg->leftSibling;
	arg->leftSibling = new;
	new->rightSibling = arg;
	setLabel(new, newLabel);

	return new;
}

/**
 * @brief Wstawia nowy wierzchołek jako prawego brata danego.
 *
 * @param arg Dany wierzchołek.
 * @param label Etykieta nowego wierzchołka.
 *
 * @return Nowy wierzchołek lub NULL w przypadku błędu alokacji.
 */
rt *
addRight(rt *arg, const char *label)
{
	rt *new = makeRT();
	if (!new) return NULL;
	char *newLabel = copyString(label, NULL);
	if (!newLabel) {free(new); return NULL;};

	*fromRightSibling(arg) = new;
	new->rightSibling = arg->rightSibling;
	arg->rightSibling = new;
	new->leftSibling = arg;
	setLabel(new, newLabel);

	return new;
}

/**
 * @brief Wstawia nowy wierzchołek jako jedyne dziecko danego.
 *
 * @param arg Dany wierzchołek.
 * @param label Etykieta nowego wierzchołka.
 *
 * @return Nowy wierzchołek lub NULL w przypadku błędu alokacji.
 */
rt*
addBelow(rt *arg, const char *label)
{
	rt *new = makeRT();
	if (!new) return NULL;
	char *newLabel = copyString(label, NULL);
	if (!newLabel) {free(new); return NULL;};

	new->leftSibling = new->rightSibling = arg;
	arg->leftChild = arg->rightChild = new;
	setLabel(new, newLabel);

	return new;
}

/**
 * @brief Wybiera dziecko, którego pierwszy znak jest zgodny z podana etykietą.
 * Jeśli takie dziecko nie istnieje, tworzone jest nowe.
 *
 * @param arg Dany wierzchołek.
 * @param label Etykieta dziecka.
 *
 * @return Określone wyżej dziecko lub NULL w przypadku błędu alokacji.
 */
rt *
addChild(rt *arg, const char *label)
{
	if (arg->leftChild == arg) {
		return addBelow(arg, label);
	} else {
		rt *child = arg->rightChild;
		for (;child != arg; child = child->rightSibling) {
			if (child->label[0] == label[0])
				return child;
			if (child->label[0] > label[0])
				break;
		}
		return child == arg ? addRight(arg->leftChild, label)
		                    : addLeft(child, label);
	}
}

/**
 * @brief Wybiera dziecko, którego pierwszy znak jest zgodny z podana etykietą.
 *
 * @param arg Dany wierzchołek.
 * @param label Etykieta dziecka.
 *
 * @return Określone wyżej dziecko lub NULL jeśli takie dziecko nie istnieje.
 */
rt *
selectChild(rt *arg, const char *label)
{
	for (rt *c = arg->rightChild; c != arg; c = c->rightSibling) {
		if (c->label[0] == label[0])
			return c;
		if (c->label[0] > label[0])
			break;
	}
	return NULL;
}

/**
 * @brief Wycina podany wierzchołek ze drzewa.
 *
 * @param arg Wycinany wierzchołek.
 *
 * @return true, jeśli wycinanie się powiodło, lub false, jeśli zostało ono
 * uniemożliwione błędem alokacji.
 */
bool
removeFromTree(rt *arg) {
	if (arg->leftChild == arg) {
		*fromLeftSibling(arg) = arg->rightSibling;
		*fromRightSibling(arg) = arg->leftSibling;
		free(arg->label);
	} else if (arg->leftChild == arg->rightChild) {
		rt *child = arg->leftChild;
		char *newLabel = mergeStrings(arg->label, child->label);
		if (!newLabel)
			return false;

		*fromLeftSibling(arg) = child;
		*fromRightSibling(arg) = child;
		child->leftSibling = arg->leftSibling;
		child->rightSibling = arg->rightSibling;

		free(arg->label);
		free(child->label);
		setLabel(child, newLabel);
	}
	return true;
}

/**
 * @brief Określa, czy podany wierzchołek jest korzeniem drzewa.
 *
 * @param arg Dany wierzchołek;
 */
static inline bool isRoot (rt *arg) {return arg->leftSibling == arg;}

/**
 * @brief Zwraca rodzica podanego wierzchołka, o ile jest on skrajnym dzieckiem.
 *
 * @param arg Podany wierzchołek.
 *
 * @return Rodzic @p arg, jeśli @p arg jest skrajnym dzieckiem
 * i nie jest korzeniem. W przeciwnym razie NULL.
 */
rt *
getParent (rt *arg)
{
	if (arg->leftSibling->rightSibling != arg)
		return arg->leftSibling;
	else if (arg->rightSibling->leftSibling != arg)
		return arg->rightSibling;
	else
		return NULL;
}

/**
 * @brief Usuwa zbędny wierzchołek z drzewa i z pamięci.
 *
 * Sprawdza, czy @p arg znajduje się w cyklu przekierowań lub jest korzeniem.
 * Jeśli nie, zwalnia jego fullWord (jeśli istnieje). Jeśli
 * ponadto @p arg posiada co najwyżej jedno dziecko, zostaje uznany za zbędny i
 * usunięty z drzewa i z pamięci, o ile nie uniemożliwią tego błędy alokacji.
 *
 * @param arg Podany wierzchołek.
 */
void
cleanup (rt* arg)
{
	if (isRoot(arg) || arg->leftRev != arg)
		return;
	if (arg->fullWord) {
		free(arg->fullWord);
		arg->fullWord = NULL;
	}
	if (arg->leftChild != arg->rightChild) {
		return;
	}

	rt *parent = getParent(arg);
	if (!removeFromTree(arg))
		return;

	free(arg);

	if (parent)
		cleanup(parent);
}

/**
 * @brief Dodaje słowo z "from" do cyklu przekierowań słowa z "to".
 *
 * @param src Dodawane słowo z drzewa "from".
 * @param fwd Słowo z drzewa "to".
 */
void addAsRev(rt *src, rt *fwd)
{
	src->leftRev = fwd;
	src->rightRev = fwd->rightRev;
	src->rightRev->leftRev = src;
	src->leftRev->rightRev = src;
	src->fwd = fwd;
}

/**
 * @brief Usuwa słowo z "from" z jego cyklu przekierowań.
 *
 * @param src Usuwane słowo z drzewa "from".
 */
void removeAsRev(rt *src)
{
	src->leftRev->rightRev = src->rightRev;
	src->rightRev->leftRev = src->leftRev;
	src->leftRev = src->rightRev = src;
}


////////////////////////////////////////////////////////////////////////////////
// Operacje na słowach

/**
 * @brief Dodaje słowo do drzewa.
 *
 * @param arg Korzeń drzewa, do którego dodawane ma zostać dodane słowo.
 * @param key Dodawane słowo.
 *
 * @return Wierzchołek odpowiadający dodawanemu słowu lub NULL w przypadku błędu
 * alokacji.
 */
rt *
addKey (rt* arg, const char *key)
{
	rt *child = addChild(arg, key);
	if (!child) return NULL;
	const char *label = child->label;
	while (*key == *label && *key && *label) {++key; ++label;}
	if (*key == '\0' && *label == '\0') {
		return child;
	} else if (*key == '\0' && *label != '\0') {
		return addAbove(child, label);
	} else if (*key != '\0' && *label != '\0') {
		rt *fork = addAbove(child, label);
		if (!fork) return NULL;
		return addChild(fork, key);
	} else {
		return addKey(child, key);
	}
}

/**
 * @brief Znajduje poddrzewo słów o podanym prefiksie.
 *
 * @param arg Korzeń przeszukiwanego drzewa.
 * @param key Szukany prefiks.
 *
 * @return Korzeń poddrzewa drzewa @p arg o prefiksie @p key, jeśli takie
 * poddrzewo istnieje, lub NULL.
 */
rt *
getBranch (rt* arg, const char *key)
{
	rt *child = selectChild(arg, key);
	if (!child)
		return NULL;

	const char *label = child->label;
	while (*key == *label && *key && *label) {++key; ++label;}
	if (!*key) {
		return child;
	} else if (!*label) {
		return getBranch(child, key);
	} else {
		return NULL;
	}
}

/**
 * @brief Usuwa podane poddrzewo z drzewa "from".
 *
 * @param arg Korzeń usuwanego poddrzewa.
 */
void
removeBranchRec (rt* arg)
{
	if (arg->fwd != NULL) {
		removeAsRev(arg);
		cleanup(arg->fwd);
		arg->fwd = NULL;
	}

	for (rt *c = arg->rightChild; c != arg;) {
		rt *tmp = c->rightSibling;
		removeBranchRec(c);
		c = tmp;
	}

	free(arg->label);
	if (arg->fullWord)
		free(arg->fullWord);
	free(arg);
}

/**
 * @brief Usuwa z danego drzewa "from" wszystkie słowa o podanym prefiksie.
 *
 * @param arg Korzeń drzewa "from".
 * @param prefix Prefix, którego wszystkie rozwinięcia mają zostać usunięte.
 */
void
removeBranch (rt* arg, const char *prefix)
{
	rt *root = getBranch(arg, prefix);
	if (!root)
		return;

	*fromLeftSibling(root) = root->rightSibling;
	*fromRightSibling(root) = root->leftSibling;
	removeBranchRec(root);
}

/**
 * @brief Usuwa całe drzewo.
 *
 * @param arg Korzeń usuwanego drzewa.
 */
void
deleteRec (rt *arg) {
	for (rt *c = arg->rightChild; c != arg;) {
		rt *tmp = c->rightSibling;
		deleteRec(c);
		c = tmp;
	}
	free(arg->label);
	if (arg->fullWord)
		free(arg->fullWord);
	free(arg);
}

////////////////////////////////////////////////////////////////////////////////
// Sortowanie leksykograficzne

/**
 * @brief Tworzy nowe drzewo sortujące.
 *
 * @param key Pierwsze słowo w nowym drzewie sortującym.
 *
 * @return Nowe drzewo sortujące lub NULL w przypadku błędu alokacji.
 */
rt *
makeSorter(const char *key)
{
	rt *sorter = makeRT();
	if (!sorter) return NULL;
	sorter->label = calloc(1,1);
	if (!sorter->label) {free(sorter); return NULL;}
	rt *k = addKey (sorter, key);
	if (!k) {deleteRec(sorter); return NULL;}
	k->fullWord = copyString(key, NULL);
	if (!k->fullWord) {deleteRec(sorter); return NULL;}
	return sorter;
}

/**
 * @brief Wypisuje do struktury PhoneNumbers wszystkie słowa zapisane w
 * drzewie w kolejności leksykograficznej.
 *
 * @param arg Drzewo do wypisania.
 * @param[out] p Struktura, do której drzewo jest wypisywane.
 */
void
prefixOrder(rt *arg, struct PhoneNumbers *p)
{
	if (arg->fullWord)
		p->data[p->size++] = arg->fullWord;
	for (rt *c = arg->rightChild; c != arg; c = c->rightSibling) {
		prefixOrder(c, p);
	}

}

void
/**
 * @brief Usuwa drzewo sortujące.
 *
 * @param arg Usuwane drzewo sortujące.
 * @see prefixOrder
 */
deleteSorter (rt *arg) {
	for (rt *c = arg->rightChild; c != arg;) {
		rt *tmp = c->rightSibling;
		deleteSorter(c);
		c = tmp;
	}
	free(arg->label);
	free(arg);
}

////////////////////////////////////////////////////////////////////////////////
// Implementacja interfejsu

struct PhoneForward *
phfwdNew(void)
{
	struct PhoneForward *new = malloc(sizeof(struct PhoneForward));
	if (!new) return NULL;
	new->from = makeRT();
	new->to = makeRT();
	if (!new->to || !new->from) goto alloc_error_1;
	new->from->label = calloc(1,1);
	new->to->label = calloc(1,1);
	if (!new->to->label || !new->from->label) goto alloc_error_2;
	return new;

alloc_error_2:
	free(new->from->label);
	free(new->to->label);
alloc_error_1:
	free(new->to);
	free(new->from);
	free(new);
	return NULL;
}

void phfwdDelete(struct PhoneForward *arg)
{
	if (!arg)
		return;
	deleteRec(arg->from);
	deleteRec(arg->to);
	free(arg);
}

bool
phfwdAdd(struct PhoneForward *arg, char const *num1, char const *num2)
{
	if (!arg || !isNumber(num1) || !isNumber(num2) || !strcmp(num1, num2))
		return false;
	rt *key1 = addKey(arg->from, num1);
	rt *key2 = addKey(arg->to, num2);
	if (!key1 || !key2) return false;
	if (key1->fwd == key2) return true;

	if (!key1->fullWord) key1->fullWord = copyString(num1, NULL);
	if (!key2->fullWord) key2->fullWord = copyString(num2, NULL);
	if (!key1->fullWord || !key2->fullWord) return false;

	rt *oldFwd = key1->fwd;
	removeAsRev(key1);
	addAsRev(key1, key2);
	if (oldFwd)
		cleanup(oldFwd);
	return true;
}

void phfwdRemove(struct PhoneForward *arg, const char *key)
{
	if (!arg) return;
	if (!isNumber(key))
		return;
	removeBranch(arg->from, key);
}

const struct PhoneNumbers *
phfwdGet(struct PhoneForward *argpf, char const *key)
{
	if (!argpf) return NULL;
	rt *arg = argpf->from;
	if (!isNumber(key)) {
		struct PhoneNumbers *new = malloc(sizeof(struct PhoneNumbers));
		if (!new) return NULL;
		new->size = 0;
		return new;
	}

	const char *bestPrefix = "";
	const char *bestSuffix = key;
	while (1) {
		if (arg->fwd) {
			bestPrefix = arg->fwd->fullWord;
			bestSuffix = key;
		}
		if (key[0] == '\0') break;

		rt *child = selectChild(arg, key);
		if (!child)
			break;
		const char *label = child->label;
		while (*key == *label && *key && *label) {++key; ++label;}
		if (label[0] == '\0') {
			arg = child;
		} else {
			break;
		}
	}
	struct PhoneNumbers *new = malloc(sizeof(struct PhoneNumbers)
	                                  + sizeof(char*));
	if (!new) return NULL;
	new->size = 1;
	new->data[0] = mergeStrings(bestPrefix, bestSuffix);
	if (!new->data[0]) {free(new); return NULL;}
	return new;
}

/**
 * @brief Wpisuje wszystkie słowa określone w phfwdReverse() do drzewa
 * sortującego.
 *
 * @param arg Drzewo "to".
 * @param key Słowo podane w phfwdReverse.
 * @param[out] acc Drzewo sortujące.
 * @param counter Obecna liczba słów zapisanych w @p acc.
 *
 * @return Zaktualizowana liczba słów w @p acc. Jeśli wystąpił błąd alokacji,
 * zwraca -1.
 */
int
reverseRev(rt *arg, const char *key, rt *acc, size_t counter) {
	for (rt *r = arg->rightRev; r != arg; r = r->rightRev) {
		char *combined = mergeStrings(r->fullWord, key);
		if (!combined) return -1;
		rt *k = addKey (acc, combined);
		if (!k) {free(combined); return -1;}
		if (!k->fullWord) {
			k->fullWord = combined;
			++counter;
		} else {
			free(combined);
		}
	}

	if (key[0] == '\0') return counter;
	rt *child = selectChild(arg, key);
	if (!child)
		return counter;

	const char *label = child->label;
	while (*key == *label && *key && *label) {++key; ++label;}
	if (label[0] == '\0') {
		return reverseRev(child, key, acc, counter);
	} else {
		return counter;
	}
}

const struct PhoneNumbers *
phfwdReverse(struct PhoneForward *arg, char const *key)
{
	if (!arg) return NULL;
	if (!isNumber(key)) {
		struct PhoneNumbers *new = malloc(sizeof(struct PhoneNumbers));
		if (!new) return NULL;
		new->size = 0;
		return new;
	}

	rt *sorter = makeSorter(key);
	if (!sorter) return NULL;
	int size = reverseRev(arg->to, key, sorter, 0) + 1;
	struct PhoneNumbers *new = malloc(sizeof(struct PhoneNumbers) +
	                                  size * sizeof(char*));
	if (size == 0 || !new) {deleteRec(sorter); free(new); return NULL;}
	new->size = 0;
	prefixOrder(sorter, new);
	deleteSorter(sorter);
	return new;
}

char const *
phnumGet(const struct PhoneNumbers *arg, size_t idx)
{
	if (!arg) return NULL;
	return idx < arg->size ? arg->data[idx] : NULL;
}

void
phnumDelete(const struct PhoneNumbers *arg)
{
	if (!arg) return;
	for (size_t i = 0; i < arg->size; ++i)
		free((void*)arg->data[i]);
	free((void*)arg);
}

static unsigned
charset (const char *arg)
{
	unsigned acc = 0;
	while (*arg) acc |= 1 << (*arg++ - '0');
	return acc;
}

static unsigned
charset_size (unsigned charset)
{
	unsigned acc = 0;
	while (charset) {
		acc += (charset & 1);
		charset >>= 1;
	}
	return acc;
}

static bool
subset (unsigned sub, unsigned super)
{
	return (sub & super) == sub;
}

static size_t power(size_t base, size_t exp) {
	size_t ret = 1;
	while (exp) {ret *= exp%2 ? base : 1; exp /= 2; base *= base;}
	return ret;
}

static size_t
nonTrivialCountRec(rt* arg, unsigned set, unsigned set_size, size_t len)
{
	if (arg->leftRev != arg)
		return power(set_size, len);

	size_t ret = 0;
	for (rt *c = arg->rightChild; c != arg; c = c->rightSibling) {
		if (subset(c->charset, set) && c->labelLength <= len)
			ret += nonTrivialCountRec(c, set, set_size, len - c->labelLength);
	}
	return ret;
}

#define max(a, b) ((a) > (b) ? (a) : (b))

size_t
phfwdNonTrivialCount(struct PhoneForward *pf, char const *set, size_t len)
{
	if (!pf || !set) return 0;
	unsigned char_set = charset(set);
	return nonTrivialCountRec(pf->to, char_set, charset_size(char_set), len);
}
