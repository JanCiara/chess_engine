#pragma once
#include "board.h"

enum TTFlag : uint8_t {
    TT_NONE = 0,
    TT_EXACT = 1,
    TT_ALPHA = 2,
    TT_BETA = 3
};

// 16 bytes = 4 entries per 64-byte cache line (minimal probe/store footprint).
struct alignas(16) TTEntry {
    U64 key = 0;
    int32_t score = 0;
    uint16_t move = 0;
    uint8_t depth = 0;
    TTFlag flag = TT_NONE;
};

static_assert(sizeof(TTEntry) == 16, "TTEntry must be 16 bytes for cache-line packing");
static_assert(alignof(TTEntry) == 16, "TTEntry must be 16-byte aligned");

void init_zobrist();
void clear_tt();
U64 compute_hash(const Board* board);

U64 zobrist_piece_key(int piece, int square);
U64 zobrist_side_key();
U64 zobrist_castle_key(int rights);
U64 zobrist_ep_key(int square);
TTFlag probe_tt(const Board* board, U64 key, int depth, int ply, int alpha, int beta, int* score, int* move);
int probe_tt_move(const Board* board, U64 key);
void store_tt(U64 key, int depth, int ply, int score, TTFlag flag, int move);
