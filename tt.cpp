#include "tt.h"

#include <random>

#define TT_SIZE (1 << 20)
#define TT_MASK (TT_SIZE - 1)
#define MATE_SCORE 49000

static U64 piece_keys[12][64];
static U64 side_key;
static U64 castle_keys[16];
static U64 ep_keys[65];

static TTEntry tt_table[TT_SIZE];

static int score_to_tt(int score, int depth) {
    if (score >= MATE_SCORE - 256) {
        return score + depth;
    }
    if (score <= -MATE_SCORE + 256) {
        return score - depth;
    }
    return score;
}

static int score_from_tt(int score, int depth) {
    if (score >= MATE_SCORE - 256) {
        return score - depth;
    }
    if (score <= -MATE_SCORE + 256) {
        return score + depth;
    }
    return score;
}

void init_zobrist() {
    std::mt19937_64 rng(0xC0FFEEULL);

    for (int piece = P; piece <= k; piece++) {
        for (int square = a1; square <= h8; square++) {
            piece_keys[piece][square] = rng();
        }
    }

    side_key = rng();

    for (int i = 0; i < 16; i++) {
        castle_keys[i] = rng();
    }

    for (int i = 0; i <= 64; i++) {
        ep_keys[i] = rng();
    }
}

void clear_tt() {
    for (int i = 0; i < TT_SIZE; i++) {
        tt_table[i] = TTEntry{};
    }
}

U64 compute_hash(const Board* board) {
    U64 hash = 0;

    for (int piece = P; piece <= k; piece++) {
        U64 bitboard = board->bitboards[piece];
        while (bitboard) {
            int square = get_LSB(bitboard);
            hash ^= piece_keys[piece][square];
            pop_bit(bitboard, square);
        }
    }

    if (board->side == BLACK) {
        hash ^= side_key;
    }

    hash ^= castle_keys[board->castle & 15];
    hash ^= ep_keys[board->en_passant == -1 ? 64 : board->en_passant];

    return hash;
}

TTFlag probe_tt(U64 key, int depth, int alpha, int beta, int* score, int* move) {
    TTEntry* entry = &tt_table[key & TT_MASK];

    if (entry->key != key) {
        *move = 0;
        return TT_NONE;
    }

    *move = entry->move;

    if (entry->depth < depth) {
        return TT_NONE;
    }

    *score = score_from_tt(entry->score, depth);

    switch (entry->flag) {
        case TT_EXACT:
            return TT_EXACT;
        case TT_ALPHA:
            if (*score <= alpha) {
                return TT_ALPHA;
            }
            break;
        case TT_BETA:
            if (*score >= beta) {
                return TT_BETA;
            }
            break;
        default:
            break;
    }

    return TT_NONE;
}

int probe_tt_move(U64 key) {
    TTEntry* entry = &tt_table[key & TT_MASK];
    if (entry->key != key) {
        return 0;
    }
    return entry->move;
}

void store_tt(U64 key, int depth, int score, TTFlag flag, int move) {
    TTEntry* entry = &tt_table[key & TT_MASK];

    entry->key = key;
    entry->depth = depth;
    entry->score = score_to_tt(score, depth);
    entry->flag = flag;
    entry->move = move;
}
