#include <iostream>
#include <vector>
#include <cstdint>
#include "defs.h"


class Board {
public:
    // 6 White, 6 Black
    U64 bitboards[12];

    // [0] - White, [1] - Black, [2] - All
    U64 occupancies[3];

    // 0 - White, 1 - Black
    int side = 0;

    // Represents the field (e.g. d7) where the en passant is possible
    int en_passant = -1; // -1 means no en passant possible

    // Bits representing the ability to castle
    // 1 (0001) - White king (WK) can short castle
    // 2 (0010) - White king (WQ) can long castle
    // 4 (0100) - White king (BK) can short castle
    // 8 (1000) - White king (BQ) can long castle
    int castle = 15;

    Board() {
        for (int i = 0; i < 12; i++) bitboards[i] = 0ULL;
        for (int i = 0; i < 3; i++) occupancies[i] = 0ULL;

        init_start_position();
    }

    void init_start_position() {
        for (int sq = a2; sq <= h2; sq++) {
            set_bit(bitboards[P], sq);
        }

    }

    void print_bitboard(U64 bb) {

    }

};

int main() {
    Board chess;

    return 0;
}