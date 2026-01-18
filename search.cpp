#include <iostream>
#include "search.h"

int best_move = 0;

#define INFINITY 50000

static int negamax(int alpha, int beta, int depth, Board* board) {
    if (depth == 0) {
        return evaluate(board);
    }

    Moves move_list[1];
    generate_moves(board, move_list);

    int legal_moves = 0;
    int best_score = -INFINITY;

    for (int i = 0; i < move_list->count; i++) {
        Board copy = *board;
        
        if (!make_move(board, move_list->moves[i], 0)) {
            continue; 
        }
        legal_moves++;

        int score = -negamax(-beta, -alpha, depth - 1, board);

        *board = copy;

        if (score > best_score) {
            best_score = score;
        }

        if (score > alpha) {
            alpha = score;
            
            if (alpha >= beta) {
                return beta;
            }
        }
    }

    if (legal_moves == 0) {
        int king_sq = get_LSB(board->bitboards[(board->side == WHITE) ? K : k]);
        
        if (is_square_attacked(king_sq, board->side ^ 1, board)) {
            return -49000 + depth; 
        } else {
            return 0;
        }
    }

    return alpha;
}

void search_position(Board* board, int depth) {
    int alpha = -INFINITY;
    int beta = INFINITY;
    
    best_move = 0;

    Moves move_list[1];
    generate_moves(board, move_list);

    for (int i = 0; i < move_list->count; i++) {
        Board copy = *board;
        
        if (!make_move(board, move_list->moves[i], 0)) continue;

        int score = -negamax(-beta, -alpha, depth - 1, board);
        
        *board = copy;

        std::cout << "info score cp " << score
                  << " move " << Board::int_to_sq(get_move_source(move_list->moves[i]))
                  << Board::int_to_sq(get_move_target(move_list->moves[i])) << std::endl;

        if (score > alpha) {
            alpha = score;
            best_move = move_list->moves[i];
        }
    }

    std::cout << "bestmove "
              << Board::int_to_sq(get_move_source(best_move)) 
              << Board::int_to_sq(get_move_target(best_move)) << std::endl;
}