#pragma once
#include "board.h"
#include "movegen.h"
#include "evaluate.h"

extern int best_move;

void search_position(Board* board, int depth);