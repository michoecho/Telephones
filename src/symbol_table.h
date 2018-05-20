#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H
#include <stdbool.h>
typedef struct RadixTree SymbolTable;
SymbolTable * newTable (void);
void deleteTable (SymbolTable *arg);
bool addMapping (SymbolTable *arg, const char *key, void *value);
void * getMapping (SymbolTable *arg, const char *key);
void removeMapping (SymbolTable* arg, const char *key);
void iter (SymbolTable* arg, void (*f)(void*));
#endif
