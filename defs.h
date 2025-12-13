#pragma once
#include <cstdint>

// --- TYPES ---
typedef uint64_t U64;

// --- MACROS ---
#define get_bit(bitboard, square) ((bitboard) & (1ULL << (square)))
#define set_bit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define pop_bit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

#define ROW(x) (x >> 3)
#define COL(x) (x & 7)

#if defined(_MSC_VER)
// Code for Visual Studio (Windows)
#include <intrin.h>
#include <nmmintrin.h>

    #define count_bits(b) _mm_popcnt_u64(b)

    inline int get_LSB(U64 b) {
        unsigned long index;
        _BitScanForward64(&index, b);
        return (int)index;
    }

    inline int get_MSB(U64 b) {
        unsigned long index;
        _BitScanReverse64(&index, b);
        return (int)index;
    }

#else
    // Code for GCC / Clang (Linux, Mac, MinGW)
    #define count_bits(b) __builtin_popcountll(b)
    #define get_LSB(b) (__builtin_ctzll(b))
    #define get_MSB(b) (63 - __builtin_clzll(b))
#endif

inline int pop_LSB(U64 &b) {
    int i = get_LSB(b);
    b &= b - 1;
    return i;
}

// --- ENUMS ---
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

// Helpers for  generating knight moves
constexpr U64 notAFile  = 0xFEFEFEFEFEFEFEFEULL;
constexpr U64 notABFile = 0xFCFCFCFCFCFCFCFCULL;
constexpr U64 notHFile  = 0x7F7F7F7F7F7F7F7FULL;
constexpr U64 notGHFile = 0x3F3F3F3F3F3F3F3FULL;