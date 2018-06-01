/** @file
 * Implementacja mapy ze stringów na wskaźniki.
 *
 * @author Michał Chojnowski <mc394134@students.mimuw.edu.pl>
 * @copyright Michał Chojnowski
 * @date 28.05.2018
 */

#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"

/**
 * Każdemu wierzcholkowi drzewa odpowiada dokładnie jedno słowo.
 * Każdy wierzchołek posiada etykietę (label). Słowo, które odpowiada
 * wierzchołkowi, jest konkatenacją wszystkich etykiet na ścieżce od korzenia
 * do tego wierzchołka włącznie. Etykiety rodzeństwa nie mają wspólnych
 * prefiksów. Rodzeństwo jest posortowane leksykograficznie względem etykiet.
 */
struct SymbolTable {
	/** Wskazuje na lewego brata wierzchołka, jeśli istnieje,
	 * lub na rodzica, a w przypadku korzenia - na siebie. */
	struct SymbolTable *leftSibling;

	/** Wskazuje na prawego brata wierzchołka, jeśli istnieje,
	 * lub na rodzica, a w przypadku korzenia - na siebie. */
	struct SymbolTable *rightSibling;

	/** Wskazuje na skrajnie prawe (a więc pierwsze od lewej) dziecko,
	 * jeśli istnieje, lub na siebie. */
	struct SymbolTable *leftChild;

	/** Wskazuje na skrajnie lewe (a więc pierwsze od prawej) dziecko,
	 * jeśli istnieje, lub na siebie. */
	struct SymbolTable *rightChild;

	/** Etykieta wierzchołka. */
	char *label;

	/** Przechowywana wartość */
	void *value;
};

/**
 * Mapa ze słów na wskaźniki oparta na strukturze "radix tree".
 */
typedef struct SymbolTable st;

/**
 * @brief Alokuje kopię stringu.
 *
 * @param begin Początek stringu.
 * @param end Wskaźnik za ostatni znak do skopiowania. Jeśli wynosi NULL, to
 * kopiowane jest cały string aż do '\0'.
 * @return Wskaźnik na kopię lub NULL w przypadku błędu alokacji.
 */
static char *
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
static char *
mergeStrings (const char *prefix, const char *suffix)
{
	char *new = malloc(strlen(prefix) + strlen(suffix) + 1);
	if (!new) return NULL;
	strcpy(new, prefix);
	strcat(new, suffix);
	return new;
}

/**
 * @brief Alokuje nowy wierzchołek drzewa słów.
 *
 * @return Wskaźnik na nowy wierzchołek lub NULL w przypadku błędu alokacji.
 */
static st *
makeRT()
{
	st *new = malloc(sizeof(st));
	if (!new) return NULL;
	*new = (st){new, new, new, new, NULL, NULL};
	return new;
}

/**
 * @brief Zwraca właściwy wskaźnik wskazujący na dany wierzchołek z jego lewego
 * brata/rodzica.
 *
 * @param arg Dany wierzchołek.
 */
static st **
fromLeftSibling(st *arg)
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
static st **
fromRightSibling(st *arg)
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
static st *
addAbove (st *arg, const char* breakpoint)
{
	st *new = makeRT();
	if (!new) return NULL;

	new->label = copyString(arg->label, breakpoint);
	char *argLabel = copyString(breakpoint, NULL);
	if (!new->label || !argLabel) goto alloc_error;

	*fromLeftSibling(arg) = new;
	*fromRightSibling(arg) = new;
	new->leftSibling = arg->leftSibling;
	new->rightSibling = arg->rightSibling;
	new->leftChild = new->rightChild = arg;
	arg->leftSibling = arg->rightSibling = new;

	free(arg->label);
	arg->label = argLabel;

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
static st *
addLeft(st *arg, const char *label)
{
	st *new = makeRT();
	if (!new) return NULL;
	char *newLabel = copyString(label, NULL);
	if (!newLabel) {free(new); return NULL;};

	*fromLeftSibling(arg) = new;
	new->leftSibling = arg->leftSibling;
	arg->leftSibling = new;
	new->rightSibling = arg;
	new->label = newLabel;

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
static st *
addRight(st *arg, const char *label)
{
	st *new = makeRT();
	if (!new) return NULL;
	char *newLabel = copyString(label, NULL);
	if (!newLabel) {free(new); return NULL;};

	*fromRightSibling(arg) = new;
	new->rightSibling = arg->rightSibling;
	arg->rightSibling = new;
	new->leftSibling = arg;
	new->label = newLabel;

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
static st*
addBelow(st *arg, const char *label)
{
	st *new = makeRT();
	if (!new) return NULL;
	char *newLabel = copyString(label, NULL);
	if (!newLabel) {free(new); return NULL;};

	new->leftSibling = new->rightSibling = arg;
	arg->leftChild = arg->rightChild = new;
	new->label = newLabel;

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
static st *
addChild(st *arg, const char *label)
{
	if (arg->leftChild == arg) {
		return addBelow(arg, label);
	} else {
		st *child = arg->rightChild;
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
static st *
selectChild(st *arg, const char *label)
{
	for (st *c = arg->rightChild; c != arg; c = c->rightSibling) {
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
static bool
removeFromTree(st *arg)
{
	if (arg->leftChild == arg) {
		*fromLeftSibling(arg) = arg->rightSibling;
		*fromRightSibling(arg) = arg->leftSibling;
		free(arg->label);
	} else if (arg->leftChild == arg->rightChild) {
		st *child = arg->leftChild;
		char *newLabel = mergeStrings(arg->label, child->label);
		if (!newLabel)
			return false;

		*fromLeftSibling(arg) = child;
		*fromRightSibling(arg) = child;
		child->leftSibling = arg->leftSibling;
		child->rightSibling = arg->rightSibling;

		free(arg->label);
		free(child->label);
		child->label = newLabel;
	}
	return true;
}

/**
 * @brief Określa, czy podany wierzchołek jest korzeniem drzewa.
 *
 * @param arg Dany wierzchołek;
 */
static inline bool isRoot (st *arg) {return arg->leftSibling == arg;}

/**
 * @brief Zwraca rodzica podanego wierzchołka, o ile jest on skrajnym dzieckiem.
 *
 * @param arg Podany wierzchołek.
 *
 * @return Rodzic @p arg, jeśli @p arg jest skrajnym dzieckiem
 * i nie jest korzeniem. W przeciwnym razie NULL.
 */
static st *
getParent (st *arg)
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
static void
cleanup (st* arg)
{
	if (arg->value != NULL || isRoot(arg) || arg->leftChild != arg->rightChild)
		return;

	st *parent = getParent(arg);
	if (!removeFromTree(arg))
		return;

	free(arg);

	if (parent)
		cleanup(parent);
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
static st *
addKey (st* arg, const char *key)
{
	st *child = addChild(arg, key);
	if (!child) return NULL;
	const char *label = child->label;
	while (*key == *label && *key && *label) {++key; ++label;}
	if (*key == '\0' && *label == '\0') {
		return child;
	} else if (*key == '\0' && *label != '\0') {
		return addAbove(child, label);
	} else if (*key != '\0' && *label != '\0') {
		st *fork = addAbove(child, label);
		if (!fork) return NULL;
		return addChild(fork, key);
	} else {
		return addKey(child, key);
	}
}

/**
 * @brief Znajduje wierzchołek odpowiadający danemu słowu.
 *
 * @param arg Korzeń przeszukiwanego drzewa.
 * @param key Szukane słowo.
 *
 * @return Wierzchołek drzewa @p arg odpowiadający @p key, lub NULL, jeśli
 * taki wierzchołek nie istnieje.
 */
static st *
getExact (st* arg, const char *key)
{
	st *child = selectChild(arg, key);
	if (!child)
		return NULL;

	const char *label = child->label;
	while (*key == *label && *key && *label) {++key; ++label;}
	if (!*key && !*label) {
		return child;
	} else if (!*label) {
		return getExact(child, key);
	} else {
		return NULL;
	}
}

st *
newSymbolTable (void)
{
	st *new = makeRT();
	if (!new) return NULL;
	new->label = calloc(1,1);
	if (!new->label) {free(new); return NULL;}
	return new;
}

void
deleteSymbolTable (st *arg)
{
	for (st *c = arg->rightChild; c != arg;) {
		st *tmp = c->rightSibling;
		deleteSymbolTable(c);
		c = tmp;
	}
	free(arg->label);
	free(arg);
}

bool
addSymbol (st *arg, const char *key, void *value)
{
	st *target = addKey(arg, key);
	if (!target) return false;
	target->value = value;
	return true;
}

void *
getSymbol (st *arg, const char *key)
{
	st *target = getExact(arg, key);
	return target ? target->value : NULL;
}

void
removeSymbol (st* arg, const char *key)
{
	st *target = getExact(arg, key);
	if (!target) return;
	target->value = NULL;
	cleanup(target);
}

void
iterSymbols (SymbolTable* arg, void (*f)(void*))
{
	for (st *c = arg->rightChild; c != arg; c = c->rightSibling)
		iterSymbols(c, f);
	if (arg->value)
		f(arg->value);
}
