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
        // White pawns
        for (int sq = a2; sq <= h2; sq++) {
            set_bit(bitboards[P], sq);
        }

        // White pieces
        set_bit(bitboards[R], a1); set_bit(bitboards[R], h1);
        set_bit(bitboards[N], b1); set_bit(bitboards[N], g1);
        set_bit(bitboards[B], c1); set_bit(bitboards[B], f1);
        set_bit(bitboards[Q], d1); set_bit(bitboards[K], e1);

        // Black pawns
        for (int sq = a7; sq <= h7; sq++) {
            set_bit(bitboards[p], sq);
        }

        // Black pieces
        set_bit(bitboards[r], a8); set_bit(bitboards[r], h8);
        set_bit(bitboards[n], b8); set_bit(bitboards[n], g8);
        set_bit(bitboards[b], c8); set_bit(bitboards[b], f8);
        set_bit(bitboards[q], d8); set_bit(bitboards[k], e8);
    }

    void update_occupancies() {
        occupancies[0] = 0ULL;
        occupancies[1] = 0ULL;
        occupancies[2] = 0ULL;

        // White
        for (int i = P; i <= K; i++) occupancies[0] |= bitboards[i];

        // Black
        for (int i = p; i <= k; i++) occupancies[1] |= bitboards[i];

        occupancies[2] = occupancies[0] | occupancies[1];
    }

    void print_bitboard(U64 bb) {
        std::cout << '\n';
        for (int rank = 7; rank >= 0; rank--) {
            for (int file = 0; file < 8; file++) {
                int square = (rank * 8) + file;
                if (get_bit(bb, square)) std::cout << " 1 ";
                else std::cout << " . ";
            }
            std::cout << '\n';
        }
        std::cout << "\n   Bitboard Value: " << bb << "\n\n";
    }
};

int main() {
    Board chess;

    return 0;
}
