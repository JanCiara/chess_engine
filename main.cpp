#include <iostream>
#include "board.h"
#include "movegen.h"

int main() {
    // 1. Inicjalizacja tablic
    init_all_attacks();

    // 2. Ustawiamy testową sytuację
    Board chess;
    chess.parseFEN("8/8/8/3p4/3R4/8/3P4/8 w - - 0 1");
    // Wieża na D4, Pion czarny na D5, Pion biały na D2 -> Wieża powinna widzieć ruchy tylko między nimi.

    std::cout << "--- TEST MAGIC BITBOARDS ---\n";
    chess.print_board();

    // 3. Pobieramy ataki wieży z pola D4 (index 27)
    // UWAGA: occupancies[2] to bitboard obu kolorów (wszystkie blokery)
    U64 attacks = get_rook_attacks(d4, chess.occupancies[2]);

    std::cout << "Ataki wiezy z pola d4:\n";
    Board::print_bitboard(attacks);

    return 0;
}