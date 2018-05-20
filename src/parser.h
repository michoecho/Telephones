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
	END,
	OOM,
	SYNTAX_ERROR,
};

struct token {
	enum tokenType type;
	char *string;
	size_t beg;
};

size_t getToken(struct token *out);

#endif
