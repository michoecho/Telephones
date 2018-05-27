#ifndef PARSER_H
#define PARSER_H
#include <stddef.h>
	
enum tokenType {
	OP_NEW,
	OP_DEL,
	OP_QUERY,
	OP_REDIR,
	IDENT,
	NUMBER,
	UNKNOWN,
	EOF_TOKEN,
	OOM_TOKEN
};

struct token {
	enum tokenType type;
	char *string;
	size_t beg;
};

void getToken(struct token *out, size_t *count);

#endif
