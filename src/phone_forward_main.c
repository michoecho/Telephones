/** @file
 * Implementacja interpretera języka opisanego w treści zadania.
 *
 * @author Michał Chojnowski <mc394134@students.mimuw.edu.pl>
 * @copyright Michał Chojnowski
 * @date 28.05.2018
 */

#include <stdio.h>
#include "scanner.h"
#include "phone_forward.h"
#include "symbol_table.h"

/**
 * Typ polecenia do wykonania przez interpreter.
 */
enum commandType {
	SWITCH, ///< Zmiana (i ew. utworzenie poprzez phfwdNew()) obecnej bazy.
	DELETE, ///< Usunięcie bazy.
	ADD, ///< Wywołanie phfwdAdd().
	REMOVE, ///< Wywołanie phfwdRemove().
	GET, ///< Wywołanie phfwdGet() i wypisanie wyniku.
	REV, ///< Wywołanie phfwdReverse() i wypisanie wyniku.
	END, ///< Brak dalszych poleceń. Zakończenie programu.
	OOM_ERROR, ///< Błąd alokacji wewnątrz parsera lub skanera.
	EOF_ERROR, ///< Nieoczekiwany koniec danych.
	SYNTAX_ERROR ///< Błąd składniowy.
};

static char *
/** @brief Zwraca nazwę operatora odpowiadającego typowi polecenia @p t.
 *
 * @param t Typ danego polecenia.
 * @return Nazwa operatora lub pusty string,
 * jeśli podany typ polecenia nie odpowiada żadnemu operatorowi.
 */
get_op_name(enum commandType t)
{
	switch (t) {
	case SWITCH: return "NEW";
	case DELETE:
	case REMOVE: return "DEL";
	case GET:
	case REV: return "?";
	case ADD: return ">";
	default: return "";
	}
}

/**
 * Struktura opisująca polecenie języka.
 */
struct command {
	enum commandType type; ///< Typ polecenia.
	char *operand1; ///< Pierwszy argument polecenia.
	char *operand2; ///< Drugi argument, lub NULL dla poleceń jednoargumentowych.

	/** Indeks pierwszego znaku operatora polecenia.
	 * Dla END, EOF_ERROR, OOM_ERROR: nieokreślone.
	 * Dla SYNTAX_ERROR: indeks pierwszego znaku błędnego tokenu.
	 */
	size_t op_offset;
};

static void
/**
 * @brief Generuje polecenie z tokenów wczytywanych przez getToken().
 * Zwiększa @p count o liczbę wczytanych znaków. Wczytuje tylko te znaki,
 * które należą do polecenia.
 *
 * @param[out] out Zwracane polecenie.
 * @param[out] count Wskaźnik na licznik wczytanych znaków.
 */
getCommand (struct command *out, size_t *count)
{
	struct token t;
	struct token t2;
	struct token t3;
	getToken(&t, count);
	out->operand1 = t.string;
	out->operand2 = NULL;
	switch (t.type) {
	case EOF_TOKEN: out->type = END; break;
	case OP_NEW:
		out->op_offset = t.beg;
		getToken(&t2, count);
		out->operand1 = t2.string;
		if (t2.type == IDENT) {
			out->type = SWITCH;
		} else {
			out->type = t2.type == OOM_TOKEN ? OOM_ERROR : SYNTAX_ERROR;
			out->op_offset = t2.beg;
		}
		break;
	case OP_DEL:
		out->op_offset = t.beg;
		getToken(&t2, count);
		out->operand1 = t2.string;
		if (t2.type == IDENT) {
			out->type = DELETE;
		} else if (t2.type == NUMBER) {
			out->type = REMOVE;
		} else {
			out->type = t2.type == OOM_TOKEN ? OOM_ERROR : SYNTAX_ERROR;
			out->op_offset = t2.beg;
		}
		break;
	case OP_QUERY:
		out->op_offset = t.beg;
		getToken(&t2, count);
		out->operand1 = t2.string;
		if (t2.type == NUMBER) {
			out->type = REV;
		} else {
			out->type = t2.type == OOM_TOKEN ? OOM_ERROR : SYNTAX_ERROR;
			out->op_offset = t2.beg;
		}
		break;
	case NUMBER:
		getToken(&t2, count);
		out->op_offset = t2.beg;
		if (t2.type == OP_REDIR) {
			getToken(&t3, count);
			out->operand2 = t3.string;
			if (t3.type == NUMBER) {
				out->type = ADD;
			} else {
				out->type = t3.type == OOM_TOKEN ? OOM_ERROR : SYNTAX_ERROR;
				out->op_offset = t3.beg;
			}
		} else if (t2.type == OP_QUERY) {
			out->type = GET;
			out->operand1 = t.string;
		} else {
			out->operand2 = t2.string;
			out->type = t2.type == OOM_TOKEN ? OOM_ERROR : SYNTAX_ERROR;
			out->op_offset = t2.beg;
		}
		break;
	case OOM_TOKEN:
		out->type = OOM_ERROR;
		break;
	default:
		out->type = SYNTAX_ERROR;
		out->op_offset = t.beg;
		break;
	}
	return;
}

/**
 * @brief Usuwa bazę. Wrapper na phfwdDelete().
 *
 * @param p Usuwana baza.
 */
static void
deleteBase(void *p)
{
	phfwdDelete(p);
}

/**
 * Stan programu.
 */
enum status {
	SUCCESS = 0, ///< Wykonanie programu zakończyło się bez błędów.
	RUNNING, ///< Program wciąż trwa.
	ERROR, ///< Wykonanie programu zakończyło się błędem.
};

/**
 * Główna pętla interpretera.
 */
int main()
{
	SymbolTable *table = newSymbolTable();
	struct PhoneForward *current = NULL;
	enum status status = RUNNING;
	size_t count = 0;
	struct command cmd;

	if (!table) {cmd.type = OOM_ERROR; status = ERROR;}
	while (status == RUNNING) {
		const struct PhoneNumbers *pn = NULL;

		getCommand(&cmd, &count);
		if (cmd.type == END) {
			status = SUCCESS;
		} else if (cmd.type == OOM_ERROR) {
			status = ERROR;
		} else if (cmd.type == SYNTAX_ERROR) {
			status = ERROR;
		} else if (cmd.type == SWITCH) {
			current = getSymbol(table, cmd.operand1);
			if (!current) {
				current = phfwdNew();
				if (!current || !addSymbol(table, cmd.operand1, current)) {
					free(current);
					status = ERROR;
				}
			}
		} else if (cmd.type == DELETE) {
			struct PhoneForward *target = getSymbol(table, cmd.operand1);
			if (target == current)
				current = NULL;
			if (target) {
				phfwdDelete(target);
				removeSymbol(table, cmd.operand1);
			} else {
				status = ERROR;
			}
		} else if (!current) {
			status = ERROR;
		} else if (cmd.type == ADD) {
			if (!phfwdAdd(current, cmd.operand1, cmd.operand2))
				status = ERROR;
		} else if (cmd.type == REMOVE) {
				phfwdRemove(current, cmd.operand1);
		} else if (cmd.type == GET) {
				pn = phfwdGet(current, cmd.operand1);
				const char *num = phnumGet(pn, 0);
				if (num)
					puts(num);
				else
					status = ERROR;
		} else if (cmd.type == REV) {
			pn = phfwdReverse(current, cmd.operand1);
			const char *num = phnumGet(pn, 0);
			if (num) {
				for (int i = 0; (num = phnumGet(pn, i)); ++i)
					puts(num);
			} else {
				status = ERROR;
			}
		}	
		free(cmd.operand1);
		free(cmd.operand2);
		phnumDelete(pn);
	}
	if (status == ERROR) {
		if (cmd.type == OOM_ERROR) {
			fprintf(stderr, "ERROR OOM\n");
		} else if (feof(stdin)) {
			fprintf(stderr, "ERROR EOF\n");
		} else if (cmd.type == SYNTAX_ERROR) {
			fprintf(stderr, "ERROR %ld\n", cmd.op_offset);
		} else {
			fprintf(stderr, "ERROR %s %ld\n", get_op_name(cmd.type),
					cmd.op_offset);
		}
	}
	if (table) {
		iterSymbols(table, deleteBase);
		deleteSymbolTable(table);
	}
	return !!status;
}
