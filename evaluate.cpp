#include "evaluate.h"
#include "movegen.h"

constexpr int PHASE_MIDGAME = 256;

// --- PIECE-SQUARE TABLES (MG) ---
const int pawn_mg[64] = {
    0,   0,   0,   0,   0,   0,   0,   0,
    5,  10,  10, -20, -20,  10,  10,   5,
    5,  -5, -10,   0,   0, -10,  -5,   5,
    0,   0,   0,  20,  20,   0,   0,   0,
    5,   5,  10,  25,  25,  10,   5,   5,
   10,  10,  20,  30,  30,  20,  10,  10,
   50,  50,  50,  50,  50,  50,  50,  50,
    0,   0,   0,   0,   0,   0,   0,   0
};

const int knight_mg[64] = {
   -50, -40, -30, -30, -30, -30, -40, -50,
   -40, -20,   0,   5,   5,   0, -20, -40,
   -30,   5,  10,  15,  15,  10,   5, -30,
   -30,   0,  15,  20,  20,  15,   0, -30,
   -30,   5,  15,  20,  20,  15,   5, -30,
   -30,   0,  10,  15,  15,  10,   0, -30,
   -40, -20,   0,   0,   0,   0, -20, -40,
   -50, -40, -30, -30, -30, -30, -40, -50
};

const int bishop_mg[64] = {
   -20, -10, -10, -10, -10, -10, -10, -20,
   -10,   5,   0,   0,   0,   0,   5, -10,
   -10,  10,  10,  10,  10,  10,  10, -10,
   -10,   0,  10,  10,  10,  10,   0, -10,
   -10,   5,   5,  10,  10,   5,   5, -10,
   -10,   0,   5,  10,  10,   5,   0, -10,
   -10,   0,   0,   0,   0,   0,   0, -10,
   -20, -10, -10, -10, -10, -10, -10, -20
};

const int rook_mg[64] = {
     0,   0,   0,   5,   5,   0,   0,   0,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
     5,  10,  10,  10,  10,  10,  10,   5,
     0,   0,   0,   0,   0,   0,   0,   0
};

const int queen_mg[64] = {
   -20, -10, -10,  -5,  -5, -10, -10, -20,
   -10,   0,   0,   0,   0,   0,   0, -10,
   -10,   0,   5,   5,   5,   5,   0, -10,
    -5,   0,   5,   5,   5,   5,   0,  -5,
     0,   0,   5,   5,   5,   5,   0,  -5,
   -10,   5,   5,   5,   5,   5,   0, -10,
   -10,   0,   5,   0,   0,   0,   0, -10,
   -20, -10, -10,  -5,  -5, -10, -10, -20
};

// King MG: encourage castling / shelter on back ranks
const int king_mg[64] = {
    20,  30,  10,   0,   0,  10,  30,  20,
    20,  20,   0,   0,   0,   0,  20,  20,
   -10, -20, -20, -20, -20, -20, -20, -10,
   -20, -30, -30, -40, -40, -30, -30, -20,
   -30, -40, -40, -50, -50, -40, -40, -30,
   -30, -40, -40, -50, -50, -40, -40, -30,
   -30, -40, -40, -50, -50, -40, -40, -30,
   -30, -40, -40, -50, -50, -40, -40, -30
};

// --- PIECE-SQUARE TABLES (EG) ---
const int pawn_eg[64] = {
    0,   0,   0,   0,   0,   0,   0,   0,
   10,  15,  15,  -5,  -5,  15,  15,  10,
   10,   0,  -5,   5,   5,  -5,   0,  10,
    5,   5,  10,  25,  25,  10,   5,   5,
   15,  15,  20,  35,  35,  20,  15,  15,
   25,  25,  30,  40,  40,  30,  25,  25,
   80,  80,  80,  80,  80,  80,  80,  80,
    0,   0,   0,   0,   0,   0,   0,   0
};

const int knight_eg[64] = {
   -60, -40, -20, -20, -20, -20, -40, -60,
   -40, -10,  10,  15,  15,  10, -10, -40,
   -20,  10,  20,  25,  25,  20,  10, -20,
   -20,  15,  25,  30,  30,  25,  15, -20,
   -20,  10,  25,  30,  30,  25,  10, -20,
   -20,  10,  20,  25,  25,  20,  10, -20,
   -40, -10,  10,  15,  15,  10, -10, -40,
   -60, -40, -20, -20, -20, -20, -40, -60
};

const int bishop_eg[64] = {
   -15, -10, -10, -10, -10, -10, -10, -15,
   -10,  10,  10,  10,  10,  10,  10, -10,
   -10,  10,  15,  15,  15,  15,  10, -10,
   -10,  10,  15,  20,  20,  15,  10, -10,
   -10,  10,  15,  20,  20,  15,  10, -10,
   -10,  10,  15,  15,  15,  15,  10, -10,
   -10,  10,  10,  10,  10,  10,  10, -10,
   -15, -10, -10, -10, -10, -10, -10, -15
};

const int rook_eg[64] = {
    10,  10,  10,  10,  10,  10,  10,  10,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     5,   5,   5,   5,   5,   5,   5,   5,
    15,  15,  15,  15,  15,  15,  15,  15,
     0,   0,   0,   0,   0,   0,   0,   0
};

const int queen_eg[64] = {
   -20, -10, -10,  -5,  -5, -10, -10, -20,
   -10,   0,   0,   0,   0,   0,   0, -10,
   -10,   0,   5,   5,   5,   5,   0, -10,
    -5,   0,   5,  10,  10,   5,   0,  -5,
     0,   0,   5,  10,  10,   5,   0,  -5,
   -10,   5,   5,   5,   5,   5,   0, -10,
   -10,   0,   5,   0,   0,   0,   0, -10,
   -20, -10, -10,  -5,  -5, -10, -10, -20
};

// King EG: centralize and stay active
const int king_eg[64] = {
   -50, -40, -30, -20, -20, -30, -40, -50,
   -30, -20, -10,   0,   0, -10, -20, -30,
   -30, -10,  20,  30,  30,  20, -10, -30,
   -30, -10,  30,  40,  40,  30, -10, -30,
   -30, -10,  30,  40,  40,  30, -10, -30,
   -30, -10,  20,  30,  30,  20, -10, -30,
   -30, -30,   0,   0,   0,   0, -30, -30,
   -50, -40, -30, -20, -20, -30, -40, -50
};

// --- TAPERED MATERIAL ---
constexpr int P_mg = 100, P_eg = 100;
constexpr int N_mg = 325, N_eg = 280;
constexpr int B_mg = 325, B_eg = 320;
constexpr int R_mg = 500, R_eg = 550;
constexpr int Q_mg = 1000, Q_eg = 900;
constexpr int K_mg = 10000, K_eg = 10000;

// --- PAWN STRUCTURE ---
constexpr int ISOLATED_PAWN_MG     = -15;
constexpr int ISOLATED_PAWN_EG     =  -8;
constexpr int DOUBLED_PAWN_MG      = -12;
constexpr int DOUBLED_PAWN_EG      =  -6;
constexpr int PASSED_PAWN_MG       =  15;
constexpr int PASSED_PAWN_EG       =  35;
constexpr int PASSED_RANK_BONUS_MG =   6;
constexpr int PASSED_RANK_BONUS_EG =  12;

// --- MOBILITY (centipawns per legal attack square) ---
constexpr int MOB_KNIGHT_MG = 4, MOB_KNIGHT_EG = 6;
constexpr int MOB_BISHOP_MG = 3, MOB_BISHOP_EG = 5;
constexpr int MOB_ROOK_MG   = 2, MOB_ROOK_EG   = 3;
constexpr int MOB_QUEEN_MG  = 1, MOB_QUEEN_EG  = 1;

// --- KING SAFETY (middlegame only) ---
constexpr int KING_ZONE_ATTACK    =  -8;
constexpr int OPEN_FILE_NEAR_KING = -18;
constexpr int PAWN_SHIELD_BONUS   =  10;

constexpr U64 FILE_A = 0x0101010101010101ULL;

#define MIRROR_SCORE(square) (square ^ 56)

static int interpolate(int mg, int eg, int phase) {
    return (mg * phase + eg * (PHASE_MIDGAME - phase)) / PHASE_MIDGAME;
}

static int calculate_phase(const Board* board) {
    int phase = 0;
    phase += count_bits(board->bitboard(N)) * 1;
    phase += count_bits(board->bitboard(B)) * 1;
    phase += count_bits(board->bitboard(R)) * 2;
    phase += count_bits(board->bitboard(Q)) * 4;
    phase += count_bits(board->bitboard(n)) * 1;
    phase += count_bits(board->bitboard(b)) * 1;
    phase += count_bits(board->bitboard(r)) * 2;
    phase += count_bits(board->bitboard(q)) * 4;

    phase = (phase * PHASE_MIDGAME + 12) / 24;
    if (phase > PHASE_MIDGAME) phase = PHASE_MIDGAME;
    return phase;
}

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

static void passed_pawn_bonus(int square, int color, int& mg, int& eg) {
    int rank = ROW(square);
    int advance = (color == WHITE) ? rank - 1 : 6 - rank;
    if (advance < 0) advance = 0;
    mg += PASSED_PAWN_MG + PASSED_RANK_BONUS_MG * advance;
    eg += PASSED_PAWN_EG + PASSED_RANK_BONUS_EG * advance;
}

static void eval_pawn_structure(U64 friendly_pawns, U64 enemy_pawns, int color,
                                int& mg, int& eg) {
    int doubled[8] = {};

    U64 bb = friendly_pawns;
    while (bb) {
        int sq = pop_LSB(bb);
        int file = COL(sq);

        doubled[file]++;
        if (is_isolated_pawn(file, friendly_pawns)) {
            mg += ISOLATED_PAWN_MG;
            eg += ISOLATED_PAWN_EG;
        }
        if (is_passed_pawn(sq, color, enemy_pawns))
            passed_pawn_bonus(sq, color, mg, eg);
    }

    for (int file = 0; file < 8; file++) {
        if (doubled[file] > 1) {
            int extra = doubled[file] - 1;
            mg += DOUBLED_PAWN_MG * extra;
            eg += DOUBLED_PAWN_EG * extra;
        }
    }
}

static void count_mobility(const Board* board, int color, int& mg, int& eg) {
    U64 occupancy = board->occupancy(2);
    U64 not_friendly = ~board->occupancy(color);

    int pieces[] = {
        (color == WHITE) ? N : n,
        (color == WHITE) ? B : b,
        (color == WHITE) ? R : r,
        (color == WHITE) ? Q : q
    };
    int mg_weights[] = {MOB_KNIGHT_MG, MOB_BISHOP_MG, MOB_ROOK_MG, MOB_QUEEN_MG};
    int eg_weights[] = {MOB_KNIGHT_EG, MOB_BISHOP_EG, MOB_ROOK_EG, MOB_QUEEN_EG};

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
            int squares = count_bits(attacks & not_friendly);
            mg += squares * mg_weights[i];
            eg += squares * eg_weights[i];
        }
    }
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
    int mg = 0;
    int eg = 0;
    int phase = calculate_phase(board);
    U64 bitboard;
    int square;

    // --- WHITE PIECES ---

    bitboard = board->bitboard(P);
    while (bitboard) {
        square = get_LSB(bitboard);
        mg += P_mg + pawn_mg[square];
        eg += P_eg + pawn_eg[square];
        pop_bit(bitboard, square);
    }

    bitboard = board->bitboard(N);
    while (bitboard) {
        square = get_LSB(bitboard);
        mg += N_mg + knight_mg[square];
        eg += N_eg + knight_eg[square];
        pop_bit(bitboard, square);
    }

    bitboard = board->bitboard(B);
    while (bitboard) {
        square = get_LSB(bitboard);
        mg += B_mg + bishop_mg[square];
        eg += B_eg + bishop_eg[square];
        pop_bit(bitboard, square);
    }

    bitboard = board->bitboard(R);
    while (bitboard) {
        square = get_LSB(bitboard);
        mg += R_mg + rook_mg[square];
        eg += R_eg + rook_eg[square];
        pop_bit(bitboard, square);
    }

    bitboard = board->bitboard(Q);
    while (bitboard) {
        square = get_LSB(bitboard);
        mg += Q_mg + queen_mg[square];
        eg += Q_eg + queen_eg[square];
        pop_bit(bitboard, square);
    }

    bitboard = board->bitboard(K);
    if (bitboard) {
        square = get_LSB(bitboard);
        mg += K_mg + king_mg[square];
        eg += K_eg + king_eg[square];
    }

    // --- BLACK PIECES ---

    bitboard = board->bitboard(p);
    while (bitboard) {
        square = get_LSB(bitboard);
        int mirrored = MIRROR_SCORE(square);
        mg -= P_mg + pawn_mg[mirrored];
        eg -= P_eg + pawn_eg[mirrored];
        pop_bit(bitboard, square);
    }

    bitboard = board->bitboard(n);
    while (bitboard) {
        square = get_LSB(bitboard);
        int mirrored = MIRROR_SCORE(square);
        mg -= N_mg + knight_mg[mirrored];
        eg -= N_eg + knight_eg[mirrored];
        pop_bit(bitboard, square);
    }

    bitboard = board->bitboard(b);
    while (bitboard) {
        square = get_LSB(bitboard);
        int mirrored = MIRROR_SCORE(square);
        mg -= B_mg + bishop_mg[mirrored];
        eg -= B_eg + bishop_eg[mirrored];
        pop_bit(bitboard, square);
    }

    bitboard = board->bitboard(r);
    while (bitboard) {
        square = get_LSB(bitboard);
        int mirrored = MIRROR_SCORE(square);
        mg -= R_mg + rook_mg[mirrored];
        eg -= R_eg + rook_eg[mirrored];
        pop_bit(bitboard, square);
    }

    bitboard = board->bitboard(q);
    while (bitboard) {
        square = get_LSB(bitboard);
        int mirrored = MIRROR_SCORE(square);
        mg -= Q_mg + queen_mg[mirrored];
        eg -= Q_eg + queen_eg[mirrored];
        pop_bit(bitboard, square);
    }

    bitboard = board->bitboard(k);
    if (bitboard) {
        square = get_LSB(bitboard);
        int mirrored = MIRROR_SCORE(square);
        mg -= K_mg + king_mg[mirrored];
        eg -= K_eg + king_eg[mirrored];
    }

    // --- PAWN STRUCTURE ---
    int pawn_mg_w = 0, pawn_eg_w = 0;
    int pawn_mg_b = 0, pawn_eg_b = 0;
    eval_pawn_structure(board->bitboard(P), board->bitboard(p), WHITE, pawn_mg_w, pawn_eg_w);
    eval_pawn_structure(board->bitboard(p), board->bitboard(P), BLACK, pawn_mg_b, pawn_eg_b);
    mg += pawn_mg_w - pawn_mg_b;
    eg += pawn_eg_w - pawn_eg_b;

    // --- MOBILITY ---
    int mob_mg_w = 0, mob_eg_w = 0;
    int mob_mg_b = 0, mob_eg_b = 0;
    count_mobility(board, WHITE, mob_mg_w, mob_eg_w);
    count_mobility(board, BLACK, mob_mg_b, mob_eg_b);
    mg += mob_mg_w - mob_mg_b;
    eg += mob_eg_w - mob_eg_b;

    // --- KING SAFETY (middlegame term) ---
    mg += eval_king_safety(board, WHITE, board->bitboard(P));
    mg -= eval_king_safety(board, BLACK, board->bitboard(p));

    int score = interpolate(mg, eg, phase);
    return (board->side() == WHITE) ? score : -score;
}
