#include <stdio.h>
#include "parser.h"
#include "phone_forward.h"
#include "symbol_table.h"

enum commandType {
	ADD_BASE,
	DEL_BASE,
	ADD_FWD,
	DEL_FWD,
	QUERY_FWD,
	QUERY_REV,
	END_CMD,
	OOM_CMD,
	SYNTAX_ERROR_CMD
};

struct command {
	enum commandType type;
	char *operand1;
	char *operand2;
	size_t op_offset;
};

size_t getCommand (struct command *out) {
	size_t count = 0;
	struct token t;
	struct token t2;
	struct token t3;
	count += getToken(&t);
	switch (t.type) {
	case END: out->type = END_CMD; break;
	case OP_NEW:
		out->op_offset = t.beg;
		count += getToken(&t2);
		if (t2.type == IDENT) {
			out->type = ADD_BASE;
			out->operand1 = t2.string;
		} else {
			out->type = SYNTAX_ERROR_CMD;
			return t2.beg;
		}
		break;
	case OP_DEL:
		out->op_offset = t.beg;
		count += getToken(&t2);
		if (t2.type == IDENT) {
			out->type = DEL_BASE;
			out->operand1 = t2.string;
		} else if (t2.type == NUMBER) {
			out->type = DEL_FWD;
			out->operand1 = t2.string;
		} else {
			out->type = SYNTAX_ERROR_CMD;
			return t2.beg;
		}
		break;
	case OP_QUERY:
		out->op_offset = t.beg;
		count += getToken(&t2);
		if (t2.type == NUMBER) {
			out->type = QUERY_REV;
			out->operand1 = t2.string;
		} else {
			out->type = SYNTAX_ERROR_CMD;
			return t2.beg;
		}
		break;
	case NUMBER:;
		size_t oldcount = count;
		count += getToken(&t2);
		out->op_offset = oldcount + t2.beg;
		if (t2.type == OP_REDIR) {
			count += getToken(&t3);
			if (t3.type == NUMBER) {
				out->type = ADD_FWD;
				out->operand1 = t.string;
				out->operand2 = t3.string;
			} else {
				out->type = SYNTAX_ERROR_CMD;
				return t3.beg;
			}
		} else if (t2.type == OP_QUERY) {
			out->type = QUERY_FWD;
			out->operand1 = t.string;
		} else {
			out->type = SYNTAX_ERROR_CMD;
			return t2.beg;
		}
		break;
	case OOM:
		out->type = OOM_CMD;
		return count;
	default:
		out->type = SYNTAX_ERROR_CMD;
		return t.beg;
	}
	return count;
}

static void
deleteBase(void *p)
{
	phfwdDelete(p);
}

int main()
{
	size_t count = 0;
	SymbolTable *table = newTable();
	struct PhoneForward *current;
	const struct PhoneNumbers *pn;
	while (true) {
		struct command cmd;
		size_t oldcount = count;
		count += getCommand(&cmd);
		switch (cmd.type) {
		case ADD_BASE:
			current = getMapping(table, cmd.operand1);
			if (!current)
				current = phfwdNew();
				addMapping(table, cmd.operand1, current);
			free(cmd.operand1);
			break;
		case DEL_BASE:;
			struct PhoneForward *target = getMapping(table, cmd.operand1);
			if (target == current)
				current = NULL;
			if (target) {
				phfwdDelete(target);
				removeMapping(table, cmd.operand1);
			} else {
				fprintf(stderr, "ERROR DEL %d\n", oldcount + cmd.op_offset);
				free(cmd.operand1);
				goto error_cleanup;
			}
			free(cmd.operand1);
			break;
		case ADD_FWD:
			if (current)
				phfwdAdd(current, cmd.operand1, cmd.operand2);
			free(cmd.operand1);
			free(cmd.operand2);
			break;
		case DEL_FWD:
			if (current)
				phfwdRemove(current, cmd.operand1);
			free(cmd.operand1);
			break;
		case QUERY_FWD:
			if (!current) break;
			pn = phfwdGet(current, cmd.operand1);
			printf("%s\n", phnumGet(pn, 0));
			phnumDelete(pn);
			free(cmd.operand1);
			break;
		case QUERY_REV:
			if (!current) break;
			pn = phfwdReverse(current, cmd.operand1);
			const char *num;
			for (int i = 0; (num = phnumGet(pn, i)); ++i)
				printf("%s\n", num);
			phnumDelete(pn);
			free(cmd.operand1);
			break;
		case END_CMD:
			goto success_cleanup;
		case OOM_CMD:
			goto error_cleanup;
		case SYNTAX_ERROR_CMD:
			if (feof(stdin))
				fprintf(stderr, "ERROR EOF\n");
			else 
				fprintf(stderr, "ERROR %d\n", count);
			goto error_cleanup;
		}
	}
success_cleanup:
	iter(table, deleteBase);
	deleteTable(table);
	return 0;
error_cleanup:
	iter(table, deleteBase);
	deleteTable(table);
	return 1;
}
