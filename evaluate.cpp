#include "evaluate.h"
#include "movegen.h"

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

// Queens: Slight preference for central control, avoid early edge development
const int queen_score[64] = {
   -20, -10, -10,  -5,  -5, -10, -10, -20,
   -10,   0,   0,   0,   0,   0,   0, -10,
   -10,   0,   5,   5,   5,   5,   0, -10,
    -5,   0,   5,   5,   5,   5,   0,  -5,
     0,   0,   5,   5,   5,   5,   0,  -5,
   -10,   5,   5,   5,   5,   5,   0, -10,
   -10,   0,   5,   0,   0,   0,   0, -10,
   -20, -10, -10,  -5,  -5, -10, -10, -20
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

// --- PAWN STRUCTURE ---
constexpr int ISOLATED_PAWN       = -15;
constexpr int DOUBLED_PAWN        = -12;
constexpr int PASSED_PAWN         =  25;
constexpr int PASSED_RANK_BONUS   =   8;

// --- MOBILITY (centipawns per legal attack square) ---
constexpr int MOB_KNIGHT = 4;
constexpr int MOB_BISHOP = 3;
constexpr int MOB_ROOK   = 2;
constexpr int MOB_QUEEN  = 1;

// --- KING SAFETY ---
constexpr int KING_ZONE_ATTACK    =  -8;
constexpr int OPEN_FILE_NEAR_KING = -18;
constexpr int PAWN_SHIELD_BONUS   =  10;

constexpr U64 FILE_A = 0x0101010101010101ULL;

#define MIRROR_SCORE(square) (square ^ 56)

static U64 file_mask(int file) {
    return FILE_A << file;
}

static bool is_isolated_pawn(int file, U64 friendly_pawns) {
    U64 adjacent = 0;
    if (file > 0) adjacent |= file_mask(file - 1);
    if (file < 7) adjacent |= file_mask(file + 1);
    return !(friendly_pawns & adjacent);
}

static bool is_passed_pawn(int square, int color, U64 enemy_pawns) {
    int file = COL(square);
    int rank = ROW(square);
    U64 block = 0;

    for (int f = file - 1; f <= file + 1; f++) {
        if (f < 0 || f > 7) continue;
        if (color == WHITE) {
            for (int r = rank + 1; r <= 7; r++)
                set_bit(block, r * 8 + f);
        } else {
            for (int r = rank - 1; r >= 0; r--)
                set_bit(block, r * 8 + f);
        }
    }
    return !(enemy_pawns & block);
}

static int passed_pawn_bonus(int square, int color) {
    int rank = ROW(square);
    int advance = (color == WHITE) ? rank - 1 : 6 - rank;
    if (advance < 0) advance = 0;
    return PASSED_PAWN + PASSED_RANK_BONUS * advance;
}

static int eval_pawn_structure(U64 friendly_pawns, U64 enemy_pawns, int color) {
    int score = 0;
    int doubled[8] = {};

    U64 bb = friendly_pawns;
    while (bb) {
        int sq = pop_LSB(bb);
        int file = COL(sq);

        doubled[file]++;
        if (is_isolated_pawn(file, friendly_pawns))
            score += ISOLATED_PAWN;
        if (is_passed_pawn(sq, color, enemy_pawns))
            score += passed_pawn_bonus(sq, color);
    }

    for (int file = 0; file < 8; file++) {
        if (doubled[file] > 1)
            score += DOUBLED_PAWN * (doubled[file] - 1);
    }
    return score;
}

static int count_mobility(const Board* board, int color) {
    U64 occupancy = board->occupancy(2);
    U64 not_friendly = ~board->occupancy(color);
    int mob = 0;

    int pieces[] = {
        (color == WHITE) ? N : n,
        (color == WHITE) ? B : b,
        (color == WHITE) ? R : r,
        (color == WHITE) ? Q : q
    };
    int weights[] = {MOB_KNIGHT, MOB_BISHOP, MOB_ROOK, MOB_QUEEN};

    for (int i = 0; i < 4; i++) {
        U64 bb = board->bitboard(pieces[i]);
        while (bb) {
            int sq = pop_LSB(bb);
            U64 attacks;
            switch (i) {
                case 0: attacks = knight_attacks[sq]; break;
                case 1: attacks = get_bishop_attacks(sq, occupancy); break;
                case 2: attacks = get_rook_attacks(sq, occupancy); break;
                default: attacks = get_queen_attacks(sq, occupancy); break;
            }
            mob += count_bits(attacks & not_friendly) * weights[i];
        }
    }
    return mob;
}

static U64 king_zone(int king_sq, int color) {
    U64 zone = king_attacks[king_sq] | (1ULL << king_sq);
    int file = COL(king_sq);
    int rank = ROW(king_sq);
    int ahead = (color == WHITE) ? rank + 1 : rank - 1;

    if (ahead >= 0 && ahead <= 7) {
        for (int f = file - 1; f <= file + 1; f++) {
            if (f >= 0 && f <= 7)
                set_bit(zone, ahead * 8 + f);
        }
    }
    return zone;
}

static int king_safety_scale(const Board* board) {
    int queens = count_bits(board->bitboard(Q)) + count_bits(board->bitboard(q));
    if (queens == 0) return 25;
    if (queens == 1) return 75;
    return 100;
}

static int eval_king_safety(const Board* board, int color, U64 friendly_pawns) {
    int king_pc = (color == WHITE) ? K : k;
    U64 king_bb = board->bitboard(king_pc);
    if (!king_bb) return 0;

    int king_sq = get_LSB(king_bb);
    int king_file = COL(king_sq);
    int score = 0;

    U64 zone = king_zone(king_sq, color);
    int enemy = color ^ 1;
    while (zone) {
        int sq = pop_LSB(zone);
        if (is_square_attacked(sq, enemy, board))
            score += KING_ZONE_ATTACK;
    }

    for (int f = king_file - 1; f <= king_file + 1; f++) {
        if (f < 0 || f > 7) continue;
        if (!(friendly_pawns & file_mask(f)))
            score += OPEN_FILE_NEAR_KING;
    }

    int shield_rank = (color == WHITE) ? ROW(king_sq) + 1 : ROW(king_sq) - 1;
    if (shield_rank >= 0 && shield_rank <= 7) {
        for (int f = king_file - 1; f <= king_file + 1; f++) {
            if (f < 0 || f > 7) continue;
            if (get_bit(friendly_pawns, shield_rank * 8 + f))
                score += PAWN_SHIELD_BONUS;
        }
    }

    return score * king_safety_scale(board) / 100;
}

int evaluate(Board* board) {
    int score = 0;
    U64 bitboard;
    int square;

    // --- WHITE PIECES ---

    bitboard = board->bitboard(P);
    while (bitboard) {
        square = get_LSB(bitboard);
        score += P_val;
        score += pawn_score[square];
        pop_bit(bitboard, square);
    }

    bitboard = board->bitboard(N);
    while (bitboard) {
        square = get_LSB(bitboard);
        score += N_val;
        score += knight_score[square];
        pop_bit(bitboard, square);
    }

    bitboard = board->bitboard(B);
    while (bitboard) {
        square = get_LSB(bitboard);
        score += B_val;
        score += bishop_score[square];
        pop_bit(bitboard, square);
    }

    bitboard = board->bitboard(R);
    while (bitboard) {
        square = get_LSB(bitboard);
        score += R_val;
        score += rook_score[square];
        pop_bit(bitboard, square);
    }

    bitboard = board->bitboard(Q);
    while (bitboard) {
        square = get_LSB(bitboard);
        score += Q_val;
        score += queen_score[square];
        pop_bit(bitboard, square);
    }

    bitboard = board->bitboard(K);
    if (bitboard) {
        square = get_LSB(bitboard);
        score += K_val;
        score += king_score[square];
    }

    // --- BLACK PIECES ---

    bitboard = board->bitboard(p);
    while (bitboard) {
        square = get_LSB(bitboard);
        score -= P_val;
        score -= pawn_score[MIRROR_SCORE(square)];
        pop_bit(bitboard, square);
    }

    bitboard = board->bitboard(n);
    while (bitboard) {
        square = get_LSB(bitboard);
        score -= N_val;
        score -= knight_score[MIRROR_SCORE(square)];
        pop_bit(bitboard, square);
    }

    bitboard = board->bitboard(b);
    while (bitboard) {
        square = get_LSB(bitboard);
        score -= B_val;
        score -= bishop_score[MIRROR_SCORE(square)];
        pop_bit(bitboard, square);
    }

    bitboard = board->bitboard(r);
    while (bitboard) {
        square = get_LSB(bitboard);
        score -= R_val;
        score -= rook_score[MIRROR_SCORE(square)];
        pop_bit(bitboard, square);
    }

    bitboard = board->bitboard(q);
    while (bitboard) {
        square = get_LSB(bitboard);
        score -= Q_val;
        score -= queen_score[MIRROR_SCORE(square)];
        pop_bit(bitboard, square);
    }

    bitboard = board->bitboard(k);
    if (bitboard) {
        square = get_LSB(bitboard);
        score -= K_val;
        score -= king_score[MIRROR_SCORE(square)];
    }

    // --- PAWN STRUCTURE ---
    score += eval_pawn_structure(board->bitboard(P), board->bitboard(p), WHITE);
    score -= eval_pawn_structure(board->bitboard(p), board->bitboard(P), BLACK);

    // --- MOBILITY ---
    score += count_mobility(board, WHITE);
    score -= count_mobility(board, BLACK);

    // --- KING SAFETY ---
    score += eval_king_safety(board, WHITE, board->bitboard(P));
    score -= eval_king_safety(board, BLACK, board->bitboard(p));

    return (board->side() == WHITE) ? score : -score;
}
