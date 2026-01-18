#include <iostream>
#include "board.h"
#include "movegen.h"

int main() {
    init_all_attacks();

    Board chess;

    // Position 2 (Kiwipete) - good for testing captures/checks
    chess.parseFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    chess.print_board();

    // Run Perft to depth 3 (should be fast)
    // Depth 4 will take a few seconds
    perft_test(3, &chess);

    return 0;
}