#include <iostream>
#include "board.h"
#include "movegen.h"

int main() {
    init_all_attacks();

    Board chess;
    // Ustawiamy piona na E2 i piona na A7 (blisko promocji)
    chess.parseFEN("8/P7/8/8/8/8/4P3/8 w - - 0 1");
    chess.print_board();

    Moves move_list[1];
    generate_moves(&chess, move_list);

    std::cout << "Znaleziono ruchow: " << move_list->count << "\n";

    // Wypisz szczegóły ruchów (dekodowanie)
    for (int i = 0; i < move_list->count; i++) {
        int move = move_list->moves[i];
        std::cout << "Ruch: "
                  << Board::int_to_sq(get_move_source(move))
                  << Board::int_to_sq(get_move_target(move))
                  << " Promocja? " << (get_move_promoted(move) ? "TAK" : "NIE")
                  << " Double? " << (get_move_double(move) ? "TAK" : "NIE")
                  << "\n";
    }

    return 0;
}