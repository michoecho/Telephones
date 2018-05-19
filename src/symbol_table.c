#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"

struct RadixTree {
	struct RadixTree *leftSibling;
	struct RadixTree *rightSibling;
	struct RadixTree *leftChild;
	struct RadixTree *rightChild;
	char *label;
	void *value;
};

typedef struct RadixTree rt;

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

static char *
mergeStrings (const char *prefix, const char *suffix)
{
	char *new = malloc(strlen(prefix) + strlen(suffix) + 1);
	if (!new) return NULL;
	strcpy(new, prefix);
	strcat(new, suffix);
	return new;
}

static rt *
makeRT()
{
	rt *new = malloc(sizeof(rt));
	if (!new) return NULL;
	*new = (rt){new, new, new, new, NULL, NULL};
	return new;
}

static rt **
fromLeftSibling(rt *arg)
{
	return arg->leftSibling->rightSibling == arg ?
		&arg->leftSibling->rightSibling : &arg->leftSibling->rightChild;
}

static rt **
fromRightSibling(rt *arg)
{
	return arg->rightSibling->leftSibling == arg ?
		&arg->rightSibling->leftSibling : &arg->rightSibling->leftChild;
}

static rt *
addAbove (rt *arg, const char* breakpoint)
{
	rt *new = makeRT();
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

static rt *
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
	new->label = newLabel;

	return new;
}

static rt *
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
	new->label = newLabel;

	return new;
}

static rt*
addBelow(rt *arg, const char *label)
{
	rt *new = makeRT();
	if (!new) return NULL;
	char *newLabel = copyString(label, NULL);
	if (!newLabel) {free(new); return NULL;};

	new->leftSibling = new->rightSibling = arg;
	arg->leftChild = arg->rightChild = new;
	new->label = newLabel;

	return new;
}

static rt *
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

static rt *
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

static bool
removeFromTree(rt *arg)
{
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
		child->label = newLabel;
	}
	return true;
}

static inline bool isRoot (rt *arg) {return arg->leftSibling == arg;}

static rt *
getParent (rt *arg)
{
	if (arg->leftSibling->rightSibling != arg)
		return arg->leftSibling;
	else if (arg->rightSibling->leftSibling != arg)
		return arg->rightSibling;
	else
		return NULL;
}

static void
cleanup (rt* arg)
{
	if (arg->value != NULL || isRoot(arg) || arg->leftChild != arg->rightChild)
		return;

	rt *parent = getParent(arg);
	if (!removeFromTree(arg))
		return;

	free(arg);

	if (parent)
		cleanup(parent);
}

static rt *
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

static rt *
getExact (rt* arg, const char *key)
{
	rt *child = selectChild(arg, key);
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

rt *
newTable (void)
{
	rt *new = makeRT();
	if (!new) return NULL;
	new->label = calloc(1,1);
	if (!new->label) {free(new); return NULL;}
	return new;
}

void
deleteTable (rt *arg)
{
	for (rt *c = arg->rightChild; c != arg;) {
		rt *tmp = c->rightSibling;
		deleteTable(c);
		c = tmp;
	}
	free(arg->label);
	free(arg);
}

bool
addMapping (rt *arg, const char *key, void *value)
{
	rt *target = addKey(arg, key);
	if (!target) return false;
	target->value = value;
	return true;
}

void *
getMapping (rt *arg, const char *key)
{
	rt *target = getExact(arg, key);
	return target ? target->value : NULL;
}

void
removeMapping (rt* arg, const char *key)
{
	rt *target = getExact(arg, key);
	if (!target) return;
	target->value = NULL;
	cleanup(target);
}
