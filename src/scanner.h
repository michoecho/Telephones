/** @file
 * Interfejs skanera języka wyspecyfikowanego w treści zadania.
 *
 * @author Michał Chojnowski <mc394134@students.mimuw.edu.pl>
 * @copyright Michał Chojnowski
 * @date 28.05.2018
 */

#ifndef PARSER_H
#define PARSER_H
#include <stddef.h>

/**
 * Rodzaj tokenu.
 */
enum tokenType {
	OP_NEW, ///< "NEW".
	OP_DEL, ///< "DEL".
	OP_QUERY, ///< "?"
	OP_REDIR, ///< ">"
	IDENT, ///< "[a-zA-Z0-9]+"
	NUMBER, ///< "[0-9]+
	EOF_TOKEN, ///< "Koniec pliku."
	UNKNOWN, ///< "Token nieprzewidziany w specyfikacji."
	OOM_TOKEN, ///< "Błąd przy alokacji pamięci na wartość tokenu".
};

/**
 * Struktura przechowująca token.
 */
struct token {
	/** Rodzaj tokenu. */
	enum tokenType type;

	/** Dla tokenów IDENT i NUMBER: tekst tokenu.
	 * Dla pozostałych rodzajów tokenów: NULL. */
	char *string;

	/** Indeks pierwszego znaku należącego do tokenu. */
	size_t beg;
};

/** @brief Skaner tokenów.
 *
 * Generuje token ze znaków wczytywanych ze standardowego wejścia i umieszcza
 * jego dane w @p out. Wczytuje i ignoruje białe znaki oraz komentarze
 * poprzedzające początek tokenu. Zwiększa licznik @p count o liczbę
 * wczytanych przez siebie znaków.  Wczytuje znaki dokładnie do ostatniego
 * znaku tokenu włącznie. Gwarantuje, że jeśli typ zwracanego tokenu jest różny
 * od EOF_TOKEN, to feof(stdin) == false.
 *
 * @param[out] out Zwracany token.
 * @param[in,out] count Wskaźnik na licznik wczytanych dotychczas znaków.
 */
void getToken(struct token *out, size_t *count);

#endif
