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

char *
get_op_name(enum commandType t)
{
	switch (t) {
	case ADD_BASE: return "NEW";
	case DEL_BASE:
	case DEL_FWD: return "DEL";
	case QUERY_FWD:
	case QUERY_REV: return "?";
	case ADD_FWD: return ">";
	default: return "";
	}
}


struct command {
	enum commandType type;
	char *operand1;
	char *operand2;
	size_t op_offset;
};

void getCommand (struct command *out, size_t *count) {
	struct token t;
	struct token t2;
	struct token t3;
	getToken(&t, count);
	switch (t.type) {
	case END: out->type = END_CMD; break;
	case OP_NEW:
		out->op_offset = t.beg;
		getToken(&t2, count);
		if (t2.type == IDENT) {
			out->type = ADD_BASE;
			out->operand1 = t2.string;
		} else {
			out->type = SYNTAX_ERROR_CMD;
			out->op_offset = t2.beg;
			return;
		}
		break;
	case OP_DEL:
		out->op_offset = t.beg;
		getToken(&t2, count);
		if (t2.type == IDENT) {
			out->type = DEL_BASE;
			out->operand1 = t2.string;
		} else if (t2.type == NUMBER) {
			out->type = DEL_FWD;
			out->operand1 = t2.string;
		} else {
			out->type = SYNTAX_ERROR_CMD;
			out->op_offset = t2.beg;
			return;
		}
		break;
	case OP_QUERY:
		out->op_offset = t.beg;
		getToken(&t2, count);
		if (t2.type == NUMBER) {
			out->type = QUERY_REV;
			out->operand1 = t2.string;
		} else {
			out->type = SYNTAX_ERROR_CMD;
			out->op_offset = t2.beg;
			return;
		}
		break;
	case NUMBER:;
		getToken(&t2, count);
		out->op_offset = t2.beg;
		if (t2.type == OP_REDIR) {
			getToken(&t3, count);
			if (t3.type == NUMBER) {
				out->type = ADD_FWD;
				out->operand1 = t.string;
				out->operand2 = t3.string;
			} else {
				out->type = SYNTAX_ERROR_CMD;
				out->op_offset = t3.beg;
				return;
			}
		} else if (t2.type == OP_QUERY) {
			out->type = QUERY_FWD;
			out->operand1 = t.string;
		} else {
			out->type = SYNTAX_ERROR_CMD;
			out->op_offset = t2.beg;
			return;
		}
		break;
	case OOM:
		out->type = OOM_CMD;
		return;
	default:
		out->type = SYNTAX_ERROR_CMD;
		out->op_offset = t.beg;
		return;
	}
	return;
}

static void
deleteBase(void *p)
{
	phfwdDelete(p);
}

#define SUCCESS 0
#define SYNTAX_ERROR 1
#define EXEC_ERROR 2

int main()
{
	size_t count = 0;
	SymbolTable *table = newTable();
	if (!table) { fprintf(stderr, "ERROR OOM\n"); return 1; }
	struct PhoneForward *current = NULL;
	int return_status = -1;
	while (true) {
		const struct PhoneNumbers *pn = NULL;
		struct command cmd = {.operand1 = NULL, .operand2 = NULL};

		getCommand(&cmd, &count);
		if (cmd.type == END_CMD) {
			return_status = SUCCESS;
		} else if (cmd.type == OOM_CMD) {
			return_status = EXEC_ERROR;
		} else if (cmd.type == SYNTAX_ERROR_CMD) {
			return_status = 1;
		} else if (cmd.type == ADD_BASE) {
			current = getMapping(table, cmd.operand1);
			if (!current) {
				current = phfwdNew();
				if (!current || !addMapping(table, cmd.operand1, current)) {
					free(current);
					return_status = EXEC_ERROR;
				}
			}
		} else if (cmd.type == DEL_BASE) {
			struct PhoneForward *target = getMapping(table, cmd.operand1);
			if (target == current)
				current = NULL;
			if (target) {
				phfwdDelete(target);
				removeMapping(table, cmd.operand1);
			} else {
				return_status = EXEC_ERROR;
			}
		} else if (!current) {
			return_status = EXEC_ERROR;
		} else if (cmd.type == ADD_FWD) {
			if (!phfwdAdd(current, cmd.operand1, cmd.operand2))
				return_status = EXEC_ERROR;
		} else if (cmd.type == DEL_FWD) {
				phfwdRemove(current, cmd.operand1);
		} else if (cmd.type == QUERY_FWD) {
				pn = phfwdGet(current, cmd.operand1);
				const char *num = phnumGet(pn, 0);
				if (num)
					printf("%s\n", num);
				else
					return_status = EXEC_ERROR;
		} else if (cmd.type == QUERY_REV) {
			pn = phfwdReverse(current, cmd.operand1);
			const char *num = phnumGet(pn, 0);
			if (num) {
				for (int i = 0; (num = phnumGet(pn, i)); ++i)
					printf("%s\n", num);
			} else {
				return_status = EXEC_ERROR;
			}
		}	
		free(cmd.operand1);
		free(cmd.operand2);
		phnumDelete(pn);
		if (return_status == SUCCESS) {
			break;
		} else if (return_status == SYNTAX_ERROR) {
			if (feof(stdin))
				fprintf(stderr, "ERROR EOF\n");
			else
				fprintf(stderr, "ERROR %ld\n", cmd.op_offset);
			break;
		} else if (return_status == EXEC_ERROR) {
			fprintf(stderr, "ERROR %s %ld\n", get_op_name(cmd.type),
					cmd.op_offset);
			break;
		}
	}
	iter(table, deleteBase);
	deleteTable(table);
	return !!return_status;
}
