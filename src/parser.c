#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "parser.h"

size_t discardComment(int *error) {
	size_t count = 0;
	if ((++count, getchar()) != '$') {*error = -1; return count;}
	while (true) {
		switch (++count, getchar()) {
		case EOF: *error = -1; return count;
		case '$':
			switch (++count, getchar()) {
			case '$': return count;
			case EOF: *error = -1; return count;
			}
		}
	}
}

size_t extractWord(char **out, int (*class)(int)) {
	int c;
	size_t count = 0;
	size_t cap = 1024;
	char *buffer = malloc(cap);
	if (!buffer) {*out = NULL; return count;}
	while (true) {
		c = getchar();
		if (!class(c)) break;
		buffer[count++] = c;
		if (count == cap) {
			char *newBuffer = realloc(buffer, (cap *= 2));
			if (!newBuffer) {free(buffer); *out = NULL; return count;}
			buffer = newBuffer;
		}
	}
	if (c != EOF) ungetc(c, stdin);
	buffer[count] = '\0';
	*out = buffer;
	return count;
}

void getToken(struct token *out, size_t *count) {
	int c;
	while (true) {
		c = getchar(); ++*count;
		if (isspace(c)) {
			continue;
		} else if (c == '$') {
			out->beg = *count;
			int error = 0;
			*count += discardComment(&error);
			if (error) {out->type = SYNTAX_ERROR; return;}
		} else {
			break;
		}
	}
	out->beg = *count;
	if (c == EOF) {
		out->type = END;
	} else if (c == '>') {
		out->type = OP_REDIR;
	} else if (c == '?') {
		out->type = OP_QUERY;
	} else if (isdigit(c)) {
		--*count; ungetc(c, stdin);
		*count += extractWord(&out->string, isdigit);
		out->type = out->string ? NUMBER : OOM;
		return;
	} else if (isalpha(c)) {
		--*count; ungetc(c, stdin);
		*count += extractWord(&out->string, isalnum);
		if (!out->string) {
			out->type = OOM;
		} else if (!strcmp("NEW", out->string)) {
			out->type = OP_NEW;
			free(out->string);
		} else if (!strcmp("DEL", out->string)) {
			out->type = OP_DEL;
			free(out->string);
		} else {
			out->type = IDENT;
		}
		return;
	} else {
		out->type = SYNTAX_ERROR;
	}
	return;
}
