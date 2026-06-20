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

// Stored scores are relative to the current node (mate distance from here),
// search scores are relative to the root (mate distance from root). Convert
// between the two using ply.
static int score_to_tt(int score, int ply) {
    if (score >= MATE_SCORE - 256) {
        return score + ply;
    }
    if (score <= -MATE_SCORE + 256) {
        return score - ply;
    }
    return score;
}

static int score_from_tt(int score, int ply) {
    if (score >= MATE_SCORE - 256) {
        return score - ply;
    }
    if (score <= -MATE_SCORE + 256) {
        return score + ply;
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
        U64 bitboard = board->bitboard(piece);
        while (bitboard) {
            int square = get_LSB(bitboard);
            hash ^= piece_keys[piece][square];
            pop_bit(bitboard, square);
        }
    }

    if (board->side() == BLACK) {
        hash ^= side_key;
    }

    hash ^= castle_keys[board->castle_rights() & 15];
    hash ^= ep_keys[board->en_passant() == -1 ? 64 : board->en_passant()];

    return hash;
}

TTFlag probe_tt(U64 key, int depth, int ply, int alpha, int beta, int* score, int* move) {
    TTEntry* entry = &tt_table[key & TT_MASK];

    if (entry->key != key) {
        *move = 0;
        return TT_NONE;
    }

    *move = entry->move;

    if (entry->depth < depth) {
        return TT_NONE;
    }

    *score = score_from_tt(entry->score, ply);

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

void store_tt(U64 key, int depth, int ply, int score, TTFlag flag, int move) {
    TTEntry* entry = &tt_table[key & TT_MASK];

    entry->key = key;
    entry->depth = depth;
    entry->score = score_to_tt(score, ply);
    entry->flag = flag;
    entry->move = move;
}
