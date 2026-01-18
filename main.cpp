#include <iostream>
#include "board.h"
#include "movegen.h"
#include "evaluate.h"
#include "search.h" // <--- Dodaj

int main() {
    init_all_attacks();
    Board chess;

    // Pozycja testowa: Czarne (ruch) mają hetmana na h8, a Białe podstawiły wieżę na a8.
    // Białe: Król e1, Wieża a8
    // Czarne: Król e8, Hetman h8
    // Czarne powinny zagrać h8a8 (zbić wieżę).
    // FEN: R3k2q/8/8/8/8/8/8/4K3 b - - 0 1

    chess.parseFEN("R3k2q/8/8/8/8/8/8/4K3 b - - 0 1");
    std::cout << "--- POZYCJA TESTOWA ---\n";
    chess.print_board();

    // Szukaj na głębokość 3 lub 4 ruchów
    std::cout << "Szukam najlepszego ruchu...\n";
    search_position(&chess, 4);

    return 0;
}