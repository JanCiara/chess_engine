#pragma once
#include <bit>
#include <cstdint>

using U64 = uint64_t;

inline bool get_bit(U64 bitboard, int square) {
    return (bitboard & (1ULL << square)) != 0;
}

inline void set_bit(U64& bitboard, int square) {
    bitboard |= (1ULL << square);
}

inline void pop_bit(U64& bitboard, int square) {
    bitboard &= ~(1ULL << square);
}

inline constexpr int ROW(int square) { return square >> 3; }
inline constexpr int COL(int square) { return square & 7; }

inline int count_bits(U64 b) {
    return static_cast<int>(std::popcount(b));
}

inline int get_LSB(U64 b) {
    return static_cast<int>(std::countr_zero(b));
}

inline int get_MSB(U64 b) {
    return static_cast<int>(std::bit_width(b) - 1);
}

inline int pop_LSB(U64& b) {
    int i = get_LSB(b);
    b &= b - 1;
    return i;
}

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

enum {
    WK = 1, WQ = 2, BK = 4, BQ = 8,

    WHITE_CASTLING = WK | WQ,
    BLACK_CASTLING = BK | BQ
};

constexpr U64 notAFile  = 0xFEFEFEFEFEFEFEFEULL;
constexpr U64 notABFile = 0xFCFCFCFCFCFCFCFCULL;
constexpr U64 notHFile  = 0x7F7F7F7F7F7F7F7FULL;
constexpr U64 notGHFile = 0x3F3F3F3F3F3F3F3FULL;

/*
    Move encoding in 32-bit int:
    0000 0000 0000 0000 0011 1111   Source Square (0-63)
    0000 0000 0000 1111 1100 0000   Target Square (0-63)
    0000 0000 1111 0000 0000 0000   Piece (co ruszamy)
    0000 1111 0000 0000 0000 0000   Promoted Piece (na co promujemy)
    0001 0000 0000 0000 0000 0000   Capture Flag (czy bicie)
    0010 0000 0000 0000 0000 0000   Double Push (pion o 2 pola)
    0100 0000 0000 0000 0000 0000   En Passant
    1000 0000 0000 0000 0000 0000   Castling
*/

inline int encode_move(int source, int target, int piece, int promoted,
                       int capture, int double_push, int enpassant, int castling) {
    return source
         | (target << 6)
         | (piece << 12)
         | (promoted << 16)
         | (capture << 20)
         | (double_push << 21)
         | (enpassant << 22)
         | (castling << 23);
}

inline int get_move_source(int move) { return move & 0x3f; }
inline int get_move_target(int move) { return (move >> 6) & 0x3f; }
inline int get_move_piece(int move) { return (move >> 12) & 0xf; }
inline int get_move_promoted(int move) { return (move >> 16) & 0xf; }
inline int get_move_capture(int move) { return move & (1 << 20); }
inline int get_move_double(int move) { return move & (1 << 21); }
inline int get_move_enpassant(int move) { return move & (1 << 22); }
inline int get_move_castling(int move) { return move & (1 << 23); }

struct Moves {
    int moves[256];
    int count = 0;
};
