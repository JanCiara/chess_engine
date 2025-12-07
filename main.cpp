#include <iostream>
#include <vector>
#include <cstdint>

typedef uint64_t U64;

#define get_bit(bitboard, square) ((bitboard) & (1ULL << square))
#define set_bit(bitboard, square) ((bitboard) |= (1ULL << square))
#define pop_bit(bitboard, square) ((bitboard) &=  ~(1ULL << square))

enum {
    a1, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8
};

enum { P, N, B, R, Q, K, p, n, b, r, q, k };

enum { WHITE, BLACK };

enum { WK = 1, WQ = 2, BK = 4, BQ = 8 };

class Board {
public:
    // 6 White, 6 Black
    U64 bitboards[12];

    // [0] - White, [1] - Black, [2] - All
    U64 uccupancies[3];

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
};

int main() {

}