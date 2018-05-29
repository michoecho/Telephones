#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "scanner.h"

/** @brief Wczytuje komentarz z stdin.
 * Zakłada, że został już wczytany pierwszy znak komentarza. Zwiększa @p count
 * o liczbę wczytanych przez siebie znaków.
 *
 * @param[out] count Wskaźnik na licznik wczytanych znaków.
 *
 * @return true, jeśli udało się wczytać poprawny komentarz. false, jeśli drugi
 * znak komentarza jest niepoprawny lub komentarz się nie kończy.
 */
static bool
discardComment(size_t *count)
{
	if ((++*count, getchar()) != '$') return false;
	while (true) {
		switch (++*count, getchar()) {
		case EOF: return false;
		case '$':
			switch (++*count, getchar()) {
			case '$': return true;
			case EOF: return false;
			}
		}
	}
}

/** @brief Wczytuje z stdin i zwraca string kolejnych znaków spełniających
 * predykat @p class (np. isalnum, isdigit).
 * Zwiększa @p count o liczbę wczytanych przez siebie znaków. 
 *
 * @param class Predykat, który mają spełniać wczytywane znaki.
 * @param[out] count Wskaźnik na licznik wczytanych znaków.
 *
 * @return Rzeczony string, lub NULL w przypadku błędu alokacji.
 */
static char*
extractWord(int (*class)(int), size_t *count)
{
	size_t oldCount = *count;
	size_t cap = 64;
	char *buffer = malloc(cap);
	if (!buffer) return NULL;

	int c;
	while (true) {
		c = getchar();
		if (!class(c)) break;
		buffer[*count - oldCount] = c;
		++*count;
		if ((*count - oldCount) == cap) {
			char *newBuffer = realloc(buffer, (cap *= 2));
			if (!newBuffer) {free(buffer); return NULL;}
			buffer = newBuffer;
		}
	}
	ungetc(c, stdin);
	if (c == EOF)
		clearerr(stdin);
	buffer[*count - oldCount] = '\0';
	return buffer;
}

void
getToken(struct token *out, size_t *count)
{
	int c;
	out->string = NULL;
	while (true) {
		c = getchar(); ++*count;
		if (isspace(c)) {
			continue;
		} else if (c == '$') {
			out->beg = *count;
			if (!discardComment(count)) {
				out->type = UNKNOWN;
				return;
			}
		} else {
			break;
		}
	}
	out->beg = *count;
	if (c == EOF) {
		out->type = EOF_TOKEN;
	} else if (c == '>') {
		out->type = OP_REDIR;
	} else if (c == '?') {
		out->type = OP_QUERY;
	} else if (isdigit(c)) {
		--*count; ungetc(c, stdin);
		out->string = extractWord(isdigit, count);
		out->type = out->string ? NUMBER : OOM_TOKEN;
		return;
	} else if (isalpha(c)) {
		--*count; ungetc(c, stdin);
		out->string = extractWord(isalnum, count);
		if (!out->string) {
			out->type = OOM_TOKEN;
		} else if (strcmp("NEW", out->string) == 0) {
			out->type = OP_NEW;
			free(out->string);
			out->string = NULL;
		} else if (strcmp("DEL", out->string) == 0) {
			out->type = OP_DEL;
			free(out->string);
			out->string = NULL;
		} else {
			out->type = IDENT;
		}
		return;
	} else {
		out->type = UNKNOWN;
	}
	return;
}
