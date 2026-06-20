#pragma once
#include "board.h"
#include "movegen.h"
#include "evaluate.h"

struct SearchLimits {
    int depth = 0;
    int movetime = 0;
    int wtime = 0;
    int btime = 0;
    int winc = 0;
    int binc = 0;
    int movestogo = 0;
    bool infinite = false;
};

extern int best_move;

void search_position(Board* board, const SearchLimits& limits);
void request_stop();
