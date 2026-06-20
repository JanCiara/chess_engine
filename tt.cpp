#include "tt.h"

#include <cmath>
#include <cstring>
#include <random>

#define TT_SIZE (1 << 20)
#define TT_MASK (TT_SIZE - 1)
#define MATE_SCORE 49000

static U64 piece_keys[12][64];
static U64 side_key;
static U64 castle_keys[16];
static U64 ep_keys[65];

alignas(64) static TTEntry tt_table[TT_SIZE];

static uint16_t encode_tt_move(int move) {
    if (move == 0) {
        return 0;
    }

    uint16_t compact = static_cast<uint16_t>(get_move_source(move)
                                           | (get_move_target(move) << 6));
    const int promo = get_move_promoted(move);
    if (promo) {
        compact |= static_cast<uint16_t>((promo & 0xF) << 12);
    }
    return compact;
}

static int decode_tt_move(const Board* board, uint16_t compact) {
    if (compact == 0) {
        return 0;
    }

    const int from = compact & 0x3F;
    const int to = (compact >> 6) & 0x3F;
    const int promo = (compact >> 12) & 0xF;

    const int piece = board->piece_at(from);
    if (piece < 0) {
        return 0;
    }

    if (promo) {
        const int capture = board->piece_at(to) >= 0 ? 1 : 0;
        return encode_move(from, to, piece, promo, capture, 0, 0, 0);
    }

    if ((piece == K || piece == k) && std::abs(COL(to) - COL(from)) == 2) {
        return encode_move(from, to, piece, 0, 0, 0, 0, 1);
    }

    if (board->en_passant() == to && (piece == P || piece == p)) {
        return encode_move(from, to, piece, 0, 1, 0, 1, 0);
    }

    const int capture = board->piece_at(to) >= 0 ? 1 : 0;
    const int double_push = (piece == P || piece == p) && std::abs(ROW(to) - ROW(from)) == 2 ? 1 : 0;
    return encode_move(from, to, piece, 0, capture, double_push, 0, 0);
}

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
    std::memset(tt_table, 0, sizeof(tt_table));
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

TTFlag probe_tt(const Board* board, U64 key, int depth, int ply, int alpha, int beta, int* score, int* move) {
    const TTEntry* entry = &tt_table[key & TT_MASK];

    if (entry->key != key) {
        *move = 0;
        return TT_NONE;
    }

    *move = decode_tt_move(board, entry->move);

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

int probe_tt_move(const Board* board, U64 key) {
    const TTEntry* entry = &tt_table[key & TT_MASK];
    if (entry->key != key) {
        return 0;
    }
    return decode_tt_move(board, entry->move);
}

void store_tt(U64 key, int depth, int ply, int score, TTFlag flag, int move) {
    TTEntry* entry = &tt_table[key & TT_MASK];

    entry->key = key;
    entry->depth = static_cast<uint8_t>(depth);
    entry->score = score_to_tt(score, ply);
    entry->flag = flag;
    entry->move = encode_tt_move(move);
}
