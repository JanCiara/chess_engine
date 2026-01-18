#include "evaluate.h"

// --- PIECE-SQUARE TABLES (PST) ---
// Pawns: Encourage center control and advancing
const int pawn_score[64] = {
    0,   0,   0,   0,   0,   0,   0,   0,
    5,  10,  10, -20, -20,  10,  10,   5,
    5,  -5, -10,   0,   0, -10,  -5,   5,
    0,   0,   0,  20,  20,   0,   0,   0,
    5,   5,  10,  25,  25,  10,   5,   5,
   10,  10,  20,  30,  30,  20,  10,  10,
   50,  50,  50,  50,  50,  50,  50,  50,
    0,   0,   0,   0,   0,   0,   0,   0
};

// Knights: Encourage center, discourage edges
const int knight_score[64] = {
   -50, -40, -30, -30, -30, -30, -40, -50,
   -40, -20,   0,   5,   5,   0, -20, -40,
   -30,   5,  10,  15,  15,  10,   5, -30,
   -30,   0,  15,  20,  20,  15,   0, -30,
   -30,   5,  15,  20,  20,  15,   5, -30,
   -30,   0,  10,  15,  15,  10,   0, -30,
   -40, -20,   0,   0,   0,   0, -20, -40,
   -50, -40, -30, -30, -30, -30, -40, -50
};

// Bishops: Encourage long diagonals and center
const int bishop_score[64] = {
   -20, -10, -10, -10, -10, -10, -10, -20,
   -10,   5,   0,   0,   0,   0,   5, -10,
   -10,  10,  10,  10,  10,  10,  10, -10,
   -10,   0,  10,  10,  10,  10,   0, -10,
   -10,   5,   5,  10,  10,   5,   5, -10,
   -10,   0,   5,  10,  10,   5,   0, -10,
   -10,   0,   0,   0,   0,   0,   0, -10,
   -20, -10, -10, -10, -10, -10, -10, -20
};

// Rooks: Encourage 7th rank and open files (simplified here)
const int rook_score[64] = {
     0,   0,   0,   5,   5,   0,   0,   0,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
     5,  10,  10,  10,  10,  10,  10,   5,
     0,   0,   0,   0,   0,   0,   0,   0
};

// King: Encourage hiding in castle (simplified)
const int king_score[64] = {
    20,  30,  10,   0,   0,  10,  30,  20,
    20,  20,   0,   0,   0,   0,  20,  20,
   -10, -20, -20, -20, -20, -20, -20, -10,
   -20, -30, -30, -40, -40, -30, -30, -20,
   -30, -40, -40, -50, -50, -40, -40, -30,
   -30, -40, -40, -50, -50, -40, -40, -30,
   -30, -40, -40, -50, -50, -40, -40, -30,
   -30, -40, -40, -50, -50, -40, -40, -30
};

// Helper to mirror square for Black (e.g., a1 becomes a8)
// Square mapping: 0..63. Black's a8 is index 56. White's a1 is 0.
// Mirror formula: square ^ 56
// Example: a1(0) ^ 56 = 56 (a8). a2(8) ^ 56 = 48 (a7).
#define MIRROR_SCORE(square) (square ^ 56)

int evaluate(Board* board) {
    int score = 0;
    U64 bitboard;
    int square;

    // --- WHITE PIECES ---
    
    // Pawns
    bitboard = board->bitboards[P];
    while (bitboard) {
        square = get_LSB(bitboard);
        score += P_val;                 // Material
        score += pawn_score[square];    // Positional
        pop_bit(bitboard, square);
    }
    
    // Knights
    bitboard = board->bitboards[N];
    while (bitboard) {
        square = get_LSB(bitboard);
        score += N_val;
        score += knight_score[square];
        pop_bit(bitboard, square);
    }

    // Bishops
    bitboard = board->bitboards[B];
    while (bitboard) {
        square = get_LSB(bitboard);
        score += B_val;
        score += bishop_score[square];
        pop_bit(bitboard, square);
    }

    // Rooks
    bitboard = board->bitboards[R];
    while (bitboard) {
        square = get_LSB(bitboard);
        score += R_val;
        score += rook_score[square];
        pop_bit(bitboard, square);
    }

    // Queens
    bitboard = board->bitboards[Q];
    while (bitboard) {
        square = get_LSB(bitboard);
        score += Q_val;
        score += bishop_score[square];
        pop_bit(bitboard, square);
    }

    // King
    bitboard = board->bitboards[K];
    if (bitboard) {
        square = get_LSB(bitboard);
        score += K_val;
        score += king_score[square];
    }

    // --- BLACK PIECES (Subtract score) ---
    // Note: We use MIRROR_SCORE(square) to access PST from Black's perspective

    // Pawns
    bitboard = board->bitboards[p];
    while (bitboard) {
        square = get_LSB(bitboard);
        score -= P_val;
        score -= pawn_score[MIRROR_SCORE(square)];
        pop_bit(bitboard, square);
    }

    // Knights
    bitboard = board->bitboards[n];
    while (bitboard) {
        square = get_LSB(bitboard);
        score -= N_val;
        score -= knight_score[MIRROR_SCORE(square)];
        pop_bit(bitboard, square);
    }

    // Bishops
    bitboard = board->bitboards[b];
    while (bitboard) {
        square = get_LSB(bitboard);
        score -= B_val;
        score -= bishop_score[MIRROR_SCORE(square)];
        pop_bit(bitboard, square);
    }

    // Rooks
    bitboard = board->bitboards[r];
    while (bitboard) {
        square = get_LSB(bitboard);
        score -= R_val;
        score -= rook_score[MIRROR_SCORE(square)];
        pop_bit(bitboard, square);
    }

    // Queens
    bitboard = board->bitboards[q];
    while (bitboard) {
        square = get_LSB(bitboard);
        score -= Q_val;
        score -= bishop_score[MIRROR_SCORE(square)];
        pop_bit(bitboard, square);
    }

    // King
    bitboard = board->bitboards[k];
    if (bitboard) {
        square = get_LSB(bitboard);
        score -= K_val;
        score -= king_score[MIRROR_SCORE(square)];
    }

    // Return score relative to the side to move
    return (board->side == WHITE) ? score : -score;
}