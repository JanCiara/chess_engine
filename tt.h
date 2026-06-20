#pragma once
#include "board.h"

enum TTFlag : int {
    TT_NONE = 0,
    TT_EXACT = 1,
    TT_ALPHA = 2,
    TT_BETA = 3
};

struct TTEntry {
    U64 key = 0;
    int depth = 0;
    int score = 0;
    int move = 0;
    TTFlag flag = TT_NONE;
};

void init_zobrist();
void clear_tt();
U64 compute_hash(const Board* board);
TTFlag probe_tt(U64 key, int depth, int alpha, int beta, int* score, int* move);
int probe_tt_move(U64 key);
void store_tt(U64 key, int depth, int score, TTFlag flag, int move);
