#pragma once
#include "board.h"
#include "defs.h"
#include "attacks_data.h"

#include <array>

inline constexpr std::size_t BISHOP_ATTACKS_SIZE = 512;
inline constexpr std::size_t ROOK_ATTACKS_SIZE = 4096;

extern const std::array<U64, 64> knight_attacks;
extern const std::array<U64, 64> king_attacks;
extern const std::array<std::array<U64, 64>, 2> pawn_attacks;

U64 get_bishop_attacks(int square, U64 occupancy);
U64 get_rook_attacks(int square, U64 occupancy);
U64 get_queen_attacks(int square, U64 occupancy);

void generate_moves(const Board* board, Moves* move_list);

int is_square_attacked(int square, int side, const Board* board);

struct Undo {
    int move = 0;
    int captured_piece = 0;
    int captured_square = 0;
    int en_passant = -1;
    int castle_rights = 15;
    int half_move_clock = 0;
    int full_move_counter = 0;
    U64 hash_key = 0;
};

int make_move(Board* board, int move, Undo* undo, int capture_only = 0);
void undo_move(Board* board, const Undo* undo);

long long perft_driver(int depth, Board *board);
long long uci_perft(Board *board, int depth);
