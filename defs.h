#pragma once
#include <bit>
#include <cstdint>

using U64 = uint64_t;

inline constexpr bool get_bit(U64 bitboard, int square) {
    return (bitboard & (1ULL << square)) != 0;
}

inline constexpr void set_bit(U64& bitboard, int square) {
    bitboard |= (1ULL << square);
}

inline constexpr void pop_bit(U64& bitboard, int square) {
    bitboard &= ~(1ULL << square);
}

inline constexpr int ROW(int square) { return square >> 3; }
inline constexpr int COL(int square) { return square & 7; }

inline constexpr int count_bits(U64 b) {
    return static_cast<int>(std::popcount(b));
}

inline constexpr int get_LSB(U64 b) {
    return static_cast<int>(std::countr_zero(b));
}

inline constexpr int get_MSB(U64 b) {
    return static_cast<int>(std::bit_width(b) - 1);
}

inline constexpr int pop_LSB(U64& b) {
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

// Leaper attack shifts (used by compile-time attack tables)
inline constexpr U64 noNoEa(U64 b) { return (b & notHFile) << 17; }
inline constexpr U64 noEaEa(U64 b) { return (b & notGHFile) << 10; }
inline constexpr U64 soEaEa(U64 b) { return (b & notGHFile) >> 6; }
inline constexpr U64 soSoEa(U64 b) { return (b & notHFile) >> 15; }
inline constexpr U64 noNoWe(U64 b) { return (b & notAFile) << 15; }
inline constexpr U64 noWeWe(U64 b) { return (b & notABFile) << 6; }
inline constexpr U64 soWeWe(U64 b) { return (b & notABFile) >> 10; }
inline constexpr U64 soSoWe(U64 b) { return (b & notAFile) >> 17; }

inline constexpr U64 noOne(U64 b) { return b << 8; }
inline constexpr U64 eaOne(U64 b) { return (b & notHFile) << 1; }
inline constexpr U64 weOne(U64 b) { return (b & notAFile) >> 1; }
inline constexpr U64 soOne(U64 b) { return b >> 8; }
inline constexpr U64 noEaOne(U64 b) { return (b & notHFile) << 9; }
inline constexpr U64 noWeOne(U64 b) { return (b & notAFile) << 7; }
inline constexpr U64 soWeOne(U64 b) { return (b & notAFile) >> 9; }
inline constexpr U64 soEaOne(U64 b) { return (b & notHFile) >> 7; }

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
