#include <iostream>
#include "search.h"

// Global variable to store the best move found
int best_move = 0;

// Infinity constant (greater than any possible mate score)
#define INFINITY 50000

// Negamax with Alpha-Beta Pruning
static int negamax(int alpha, int beta, int depth, Board* board) {
    
    // Leaf node: return static evaluation
    if (depth == 0) {
        return evaluate(board);
    }

    Moves move_list[1];
    generate_moves(board, move_list);

    int legal_moves = 0;
    int best_score = -INFINITY;

    for (int i = 0; i < move_list->count; i++) {
        Board copy = *board;
        
        // Make move (skip if illegal)
        if (!make_move(board, move_list->moves[i], 0)) {
            continue; 
        }
        legal_moves++;

        // Recursive call (negate score, swap alpha/beta)
        int score = -negamax(-beta, -alpha, depth - 1, board);

        // Restore board
        *board = copy;

        // Better score found
        if (score > best_score) {
            best_score = score;
        }

        // Alpha update
        if (score > alpha) {
            alpha = score;
            
            // Beta Cutoff (Fail-hard)
            if (alpha >= beta) {
                return beta;
            }
        }
    }

    // Check for Mate or Stalemate if no legal moves
    if (legal_moves == 0) {
        int king_sq = get_LSB(board->bitboards[(board->side == WHITE) ? K : k]);
        
        // If King is in check -> Checkmate
        // We add 'depth' to score to prefer faster mates
        if (is_square_attacked(king_sq, board->side ^ 1, board)) {
            return -49000 + depth; 
        } else {
            return 0; // Stalemate
        }
    }

    return alpha;
}

// Main search driver
void search_position(Board* board, int depth) {
    int alpha = -INFINITY;
    int beta = INFINITY;
    
    best_move = 0;

    Moves move_list[1];
    generate_moves(board, move_list);

    // Iterate through root moves
    for (int i = 0; i < move_list->count; i++) {
        Board copy = *board;
        
        if (!make_move(board, move_list->moves[i], 0)) continue;

        // Start search
        int score = -negamax(-beta, -alpha, depth - 1, board);
        
        *board = copy;

        // Print search info (UCI style)
        std::cout << "info score cp " << score 
                  << " move " << Board::int_to_sq(get_move_source(move_list->moves[i]))
                  << Board::int_to_sq(get_move_target(move_list->moves[i])) << "\n";

        // Update best move found so far
        if (score > alpha) {
            alpha = score;
            best_move = move_list->moves[i];
        }
    }

    // Print final best move
    std::cout << "bestmove " 
              << Board::int_to_sq(get_move_source(best_move)) 
              << Board::int_to_sq(get_move_target(best_move)) << "\n";
}