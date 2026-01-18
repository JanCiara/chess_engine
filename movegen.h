#pragma once
#include "board.h"
#include "defs.h"

extern U64 bishop_attacks[64][512]; // 512 = 2^9
extern U64 rook_attacks[64][4096];  // 4096 = 2^12

extern U64 knight_attacks[64];
extern U64 king_attacks[64];
extern U64 pawn_attacks[2][64];

void init_all_attacks();

U64 get_bishop_attacks(int square, U64 occupancy);
U64 get_rook_attacks(int square, U64 occupancy);
U64 get_queen_attacks(int square, U64 occupancy);

void generate_moves(const Board* board, Moves* move_list);

int is_square_attacked(int square, int side, const Board* board);