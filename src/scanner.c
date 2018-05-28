#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "scanner.h"

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
	size_t cap = 64;
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
	if (c == EOF)
		clearerr(stdin);
	ungetc(c, stdin);
	buffer[count] = '\0';
	*out = buffer;
	return count;
}

void getToken(struct token *out, size_t *count) {
	int c;
	out->string = NULL;
	while (true) {
		c = getchar(); ++*count;
		if (isspace(c)) {
			continue;
		} else if (c == '$') {
			out->beg = *count;
			int error = 0;
			*count += discardComment(&error);
			if (error) {out->type = UNKNOWN; return;}
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
		*count += extractWord(&out->string, isdigit);
		out->type = out->string ? NUMBER : OOM_TOKEN;
		return;
	} else if (isalpha(c)) {
		--*count; ungetc(c, stdin);
		*count += extractWord(&out->string, isalnum);
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
