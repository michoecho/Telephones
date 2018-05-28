#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H
#include <stdbool.h>
typedef struct RadixTree SymbolTable;
SymbolTable * newSymbolTable (void);
void deleteSymbolTable (SymbolTable *arg);
bool addSymbol (SymbolTable *arg, const char *key, void *value);
void * getSymbol (SymbolTable *arg, const char *key);
void removeSymbol (SymbolTable* arg, const char *key);
void iterSymbols (SymbolTable* arg, void (*f)(void*));
#endif
