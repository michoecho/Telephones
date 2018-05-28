/** @file
 * Interfejs mapy ze stringów na wskaźniki.
 *
 * @author Michał Chojnowski <mc394134@students.mimuw.edu.pl>
 * @copyright Michał Chojnowski
 * @date 28.05.2018
 */

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H
#include <stdbool.h>

/**
 * Mapa ze słów (stringów) na wskaźniki oparta na strukturze "radix tree".
 */
typedef struct SymbolTable SymbolTable;

/**
 * @brief Tworzy nową mapę.
 *
 * @return Nowa mapa lub NULL w przypadku błędu alokacji.
 */
SymbolTable * newSymbolTable(void);


/**
 * @brief Usuwa mapę.
 *
 * @param arg Usuwana mapa.
 */
void deleteSymbolTable (SymbolTable *arg);

/**
 * @brief Dodaje klucz i odpowiadającą mu wartość do mapy. Jeśli mapa
 * zawiera już ten klucz, odpowiadająca mu wartość jest nadpisywana.
 *
 * @param arg Zmieniana mapa.
 * @param key Dodawany klucz.
 * @param value Wartość odpowiadająca kluczowi @p key.
 *
 * @return true, jeśli dodawanie się powiedzie, lub false w przypadku błędu alokacji.
 */
bool addSymbol (SymbolTable *arg, const char *key, void *value);

/**
 * @brief Zwraca wartość odpowiadającą kluczowi.
 *
 * @param arg Przeszukiwana mapa.
 * @param key Szukany klucz.
 *
 * @return Wartość przypisana w mapie @p arg do klucza @p key, lub NULL,
 * jeśli klucz @p nie należy do mapy @arg.
 */
void * getSymbol (SymbolTable *arg, const char *key);

/**
 * @brief Usuwa klucz z mapy. Jeśli klucz nie należy do mapy, nie robi nic.
 *
 * @param arg Zmieniana mapa.
 * @param key Usuwany klucz.
 */
void removeSymbol (SymbolTable *arg, const char *key);

/**
 * @brief Wywołuje daną funkcję na każdej przechowywanej wartości.
 *
 * Dla każdej pary <klucz, wartość> należącej do mapy wywołuje
 * funkcję @p f na wartości. Kolejność wywoływania jest zgodna z rosnącym
 * uporządkowaniem leksykograficznym kluczy.
 *
 * @param arg Przetwarzana mapa.
 * @param f Wywoływana funkcja.
 */
void iterSymbols (SymbolTable *arg, void (*f)(void*));
#endif
