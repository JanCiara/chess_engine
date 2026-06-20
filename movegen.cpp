#include "movegen.h"
#include "tt.h"

#include <array>
#include <iostream>

int is_square_attacked(int square, int side, const Board *board);
void undo_move(Board* board, const Undo* undo);

namespace {

constexpr std::array<U64, 64> make_knight_attacks() {
    std::array<U64, 64> attacks{};
    for (int sq = 0; sq < 64; sq++) {
        const U64 bb = 1ULL << sq;
        attacks[sq] = noNoEa(bb) | noEaEa(bb) |
                      soEaEa(bb) | soSoEa(bb) |
                      noNoWe(bb) | noWeWe(bb) |
                      soWeWe(bb) | soSoWe(bb);
    }
    return attacks;
}

constexpr std::array<U64, 64> make_king_attacks() {
    std::array<U64, 64> attacks{};
    for (int sq = 0; sq < 64; sq++) {
        const U64 bb = 1ULL << sq;
        attacks[sq] = noOne(bb) | eaOne(bb) | weOne(bb) | soOne(bb) |
                      noEaOne(bb) | noWeOne(bb) |
                      soEaOne(bb) | soWeOne(bb);
    }
    return attacks;
}

constexpr std::array<std::array<U64, 64>, 2> make_pawn_attacks() {
    std::array<std::array<U64, 64>, 2> attacks{};
    for (int sq = 0; sq < 64; sq++) {
        const U64 bb = 1ULL << sq;
        attacks[WHITE][sq] = noEaOne(bb) | noWeOne(bb);
        attacks[BLACK][sq] = soEaOne(bb) | soWeOne(bb);
    }
    return attacks;
}

}  // namespace

const std::array<U64, 64> knight_attacks = make_knight_attacks();
const std::array<U64, 64> king_attacks = make_king_attacks();
const std::array<std::array<U64, 64>, 2> pawn_attacks = make_pawn_attacks();

constexpr std::array<int, 64> CASTLING_RIGHTS = {
    13, 15, 15, 15, 12, 15, 15, 14,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    7, 15, 15, 15, 3, 15, 15, 11
};

static U64 get_bishop_attacks_impl(int square, U64 occupancy) {
    occupancy &= BISHOP_MASKS[square];
    occupancy *= BISHOP_MAGIC_NUMBERS[square];
    occupancy >>= 64 - BISHOP_RELEVANT_BITS[square];
    return bishop_attack_entry(square, static_cast<int>(occupancy));
}

static U64 get_rook_attacks_impl(int square, U64 occupancy) {
    occupancy &= ROOK_MASKS[square];
    occupancy *= ROOK_MAGIC_NUMBERS[square];
    occupancy >>= 64 - ROOK_RELEVANT_BITS[square];
    return rook_attack_entry(square, static_cast<int>(occupancy));
}

U64 get_bishop_attacks(int square, U64 occupancy) {
    return get_bishop_attacks_impl(square, occupancy);
}

U64 get_rook_attacks(int square, U64 occupancy) {
    return get_rook_attacks_impl(square, occupancy);
}

U64 get_queen_attacks(int square, U64 occupancy) {
    return get_bishop_attacks(square, occupancy) | get_rook_attacks(square, occupancy);
}
static void add_move(Moves *move_list, int move) {
    move_list->moves[move_list->count] = move;
    move_list->count++;
}

static void add_promotion_moves(Moves *move_list, int source, int target, int pawn, int capture) {
    static const int white_promos[] = {Q, R, B, N};
    static const int black_promos[] = {q, r, b, n};
    const int *promos = (pawn == P) ? white_promos : black_promos;

    for (int i = 0; i < 4; i++) {
        add_move(move_list, encode_move(source, target, pawn, promos[i], capture, 0, 0, 0));
    }
}

static void generate_pawn_moves(const Board *board, Moves *move_list, int side,
                                U64 occupancy, U64 enemy_occupancy) {
    const int pawn = (side == WHITE) ? P : p;
    const int push = (side == WHITE) ? 8 : -8;
    const int double_push = (side == WHITE) ? 16 : -16;
    const int promo_rank_lo = (side == WHITE) ? a7 : a2;
    const int promo_rank_hi = (side == WHITE) ? h7 : h2;
    const int start_rank_lo = (side == WHITE) ? a2 : a7;
    const int start_rank_hi = (side == WHITE) ? h2 : h7;

    U64 bitboard = board->bitboard(pawn);
    U64 en_passant_bit = (board->en_passant() != -1) ? (1ULL << board->en_passant()) : 0ULL;

    while (bitboard) {
        int source_square = get_LSB(bitboard);
        int target_square = source_square + push;

        bool push_valid = (side == WHITE) ? !(target_square > h8) : !(target_square < a1);
        if (push_valid && !get_bit(occupancy, target_square)) {
            if (source_square >= promo_rank_lo && source_square <= promo_rank_hi) {
                add_promotion_moves(move_list, source_square, target_square, pawn, 0);
            } else {
                add_move(move_list, encode_move(source_square, target_square, pawn, 0, 0, 0, 0, 0));
                if (source_square >= start_rank_lo && source_square <= start_rank_hi &&
                    !get_bit(occupancy, source_square + double_push)) {
                    add_move(move_list, encode_move(source_square, source_square + double_push, pawn, 0, 0, 1, 0, 0));
                }
            }
        }

        U64 valid_captures = pawn_attacks[side][source_square] & (enemy_occupancy | en_passant_bit);
        while (valid_captures) {
            target_square = get_LSB(valid_captures);
            int en_passant_flag = (target_square == board->en_passant()) ? 1 : 0;

            if (source_square >= promo_rank_lo && source_square <= promo_rank_hi) {
                add_promotion_moves(move_list, source_square, target_square, pawn, 1);
            } else {
                add_move(move_list, encode_move(source_square, target_square, pawn, 0, 1, 0, en_passant_flag, 0));
            }
            pop_bit(valid_captures, target_square);
        }
        pop_bit(bitboard, source_square);
    }
}

struct CastlingOption {
    int right;
    int king_from;
    int king_to;
    int empty_squares[3];
    int empty_count;
    int pass_squares[3];
    int pass_count;
};

static void generate_castling_moves(const Board *board, Moves *move_list, int side,
                                    U64 occupancy, int king_piece, int enemy) {
    if (!board->bitboard(king_piece)) return;

    static const CastlingOption white_options[] = {
        {WK, e1, g1, {f1, g1}, 2, {e1, f1, g1}, 3},
        {WQ, e1, c1, {d1, c1, b1}, 3, {e1, d1, c1}, 3},
    };
    static const CastlingOption black_options[] = {
        {BK, e8, g8, {f8, g8}, 2, {e8, f8, g8}, 3},
        {BQ, e8, c8, {d8, c8, b8}, 3, {e8, d8, c8}, 3},
    };

    const CastlingOption *options = (side == WHITE) ? white_options : black_options;
    const int option_count = 2;

    for (int i = 0; i < option_count; i++) {
        const CastlingOption &opt = options[i];
        if (!(board->castle_rights() & opt.right)) continue;

        bool blocked = false;
        for (int j = 0; j < opt.empty_count; j++) {
            if (get_bit(occupancy, opt.empty_squares[j])) {
                blocked = true;
                break;
            }
        }
        if (blocked) continue;

        bool attacked = false;
        for (int j = 0; j < opt.pass_count; j++) {
            if (is_square_attacked(opt.pass_squares[j], enemy, board)) {
                attacked = true;
                break;
            }
        }
        if (attacked) continue;

        add_move(move_list, encode_move(opt.king_from, opt.king_to, king_piece, 0, 0, 0, 0, 1));
    }
}

static U64 get_piece_attacks(int piece, int square, U64 occupancy) {
    switch (piece) {
        case N:
        case n:
            return knight_attacks[square];
        case B:
        case b:
            return get_bishop_attacks(square, occupancy);
        case R:
        case r:
            return get_rook_attacks(square, occupancy);
        case Q:
        case q:
            return get_queen_attacks(square, occupancy);
        case K:
        case k:
            return king_attacks[square];
        default:
            return 0ULL;
    }
}

int is_square_attacked(int square, int side, const Board *board) {
    if ((side == WHITE) && (pawn_attacks[BLACK][square] & board->bitboard(P))) return 1;
    if ((side == BLACK) && (pawn_attacks[WHITE][square] & board->bitboard(p))) return 1;

    if (knight_attacks[square] & ((side == WHITE) ? board->bitboard(N) : board->bitboard(n))) return 1;

    if (king_attacks[square] & ((side == WHITE) ? board->bitboard(K) : board->bitboard(k))) return 1;

    U64 bishop_targets = get_bishop_attacks(square, board->occupancy(2));
    U64 diagonal_attackers = (side == WHITE)
                                 ? (board->bitboard(B) | board->bitboard(Q))
                                 : (board->bitboard(b) | board->bitboard(q));
    if (bishop_targets & diagonal_attackers) return 1;

    U64 rook_targets = get_rook_attacks(square, board->occupancy(2));
    U64 straight_attackers = (side == WHITE)
                                 ? (board->bitboard(R) | board->bitboard(Q))
                                 : (board->bitboard(r) | board->bitboard(q));
    if (rook_targets & straight_attackers) return 1;

    return 0;
}

void generate_moves(const Board *board, Moves *move_list) {
    move_list->count = 0;

    int source_square, target_square;
    int side = board->side();

    U64 bitboard, attacks;
    U64 occupancy = board->occupancy(2);
    U64 enemy_occupancy = (side == WHITE) ? board->occupancy(BLACK) : board->occupancy(WHITE);
    U64 friendly_occupancy = (side == WHITE) ? board->occupancy(WHITE) : board->occupancy(BLACK);

    generate_pawn_moves(board, move_list, side, occupancy, enemy_occupancy);
    generate_castling_moves(board, move_list, side, occupancy,
                            (side == WHITE) ? K : k, side ^ 1);

    int start_piece = (side == WHITE) ? N : n;
    int end_piece = (side == WHITE) ? K : k;

    for (int piece = start_piece; piece <= end_piece; piece++) {
        bitboard = board->bitboard(piece);

        while (bitboard) {
            source_square = get_LSB(bitboard);

            attacks = get_piece_attacks(piece, source_square, occupancy);

            attacks &= ~friendly_occupancy;

            while (attacks) {
                target_square = get_LSB(attacks);
                int capture = get_bit(enemy_occupancy, target_square) ? 1 : 0;
                add_move(move_list, encode_move(source_square, target_square, piece, 0, capture, 0, 0, 0));
                pop_bit(attacks, target_square);
            }
            pop_bit(bitboard, source_square);
        }
    }
}

int make_move(Board* board, int move, Undo* undo, int capture_only) {
    if (capture_only && !get_move_capture(move)) {
        return 0;
    }

    const int source_square = get_move_source(move);
    const int target_square = get_move_target(move);
    const int piece = get_move_piece(move);
    const int promoted_piece = get_move_promoted(move);
    const int capture = get_move_capture(move);
    const int double_push = get_move_double(move);
    const int enpassant = get_move_enpassant(move);
    const int castling = get_move_castling(move);

    undo->move = move;
    undo->hash_key = board->hash_key();
    undo->en_passant = board->en_passant();
    undo->castle_rights = board->castle_rights();
    undo->half_move_clock = board->half_move_clock();
    undo->full_move_counter = board->full_move_counter();
    undo->captured_piece = -1;
    undo->captured_square = 0;

    if (enpassant) {
        undo->captured_square = (board->side() == WHITE) ? target_square - 8 : target_square + 8;
        undo->captured_piece = (board->side() == WHITE) ? p : P;
    } else if (capture) {
        undo->captured_square = target_square;
        const int victim = board->piece_at(target_square);
        if (victim >= 0) {
            undo->captured_piece = victim;
        }
    }

    board->pop_piece_bit(piece, source_square);
    board->set_piece_bit(piece, target_square);

    if (capture && !enpassant) {
        const int start_piece = (board->side() == WHITE) ? p : P;
        const int end_piece = (board->side() == WHITE) ? k : K;

        for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++) {
            if (board->piece_on(bb_piece, target_square)) {
                board->pop_piece_bit(bb_piece, target_square);
                break;
            }
        }
    }

    if (promoted_piece) {
        board->pop_piece_bit((board->side() == WHITE) ? P : p, target_square);
        board->set_piece_bit(promoted_piece, target_square);
    }

    if (enpassant) {
        if (board->side() == WHITE) {
            board->pop_piece_bit(p, target_square - 8);
        } else {
            board->pop_piece_bit(P, target_square + 8);
        }
    }

    board->clear_en_passant();

    if (double_push) {
        if (board->side() == WHITE) {
            board->set_en_passant(source_square + 8);
        } else {
            board->set_en_passant(source_square - 8);
        }
    }

    if (castling) {
        switch (target_square) {
            case g1:
                board->pop_piece_bit(R, h1);
                board->set_piece_bit(R, f1);
                break;
            case c1:
                board->pop_piece_bit(R, a1);
                board->set_piece_bit(R, d1);
                break;
            case g8:
                board->pop_piece_bit(r, h8);
                board->set_piece_bit(r, f8);
                break;
            case c8:
                board->pop_piece_bit(r, a8);
                board->set_piece_bit(r, d8);
                break;
        }
    }

    board->mask_castle_rights(CASTLING_RIGHTS[source_square]);
    board->mask_castle_rights(CASTLING_RIGHTS[target_square]);
    board->update_occupancies();

    if (capture || piece == P || piece == p || promoted_piece) {
        board->reset_half_move_clock();
    } else {
        board->increment_half_move_clock();
    }

    if (board->side() == BLACK) {
        board->increment_full_move_counter();
    }

    board->flip_side();

    const int side_moved = board->side() ^ 1;
    const int king_bitboard_index = (side_moved == WHITE) ? K : k;
    const int king_square = get_LSB(board->bitboard(king_bitboard_index));

    if (is_square_attacked(king_square, board->side(), board)) {
        undo_move(board, undo);
        return 0;
    }

    board->set_hash_key(compute_hash(board));
    return 1;
}

void undo_move(Board* board, const Undo* undo) {
    const int move = undo->move;
    const int source = get_move_source(move);
    const int target = get_move_target(move);
    const int piece = get_move_piece(move);
    const int promoted = get_move_promoted(move);
    const int castling = get_move_castling(move);

    board->flip_side();

    if (castling) {
        switch (target) {
            case g1:
                board->pop_piece_bit(R, f1);
                board->set_piece_bit(R, h1);
                break;
            case c1:
                board->pop_piece_bit(R, d1);
                board->set_piece_bit(R, a1);
                break;
            case g8:
                board->pop_piece_bit(r, f8);
                board->set_piece_bit(r, h8);
                break;
            case c8:
                board->pop_piece_bit(r, d8);
                board->set_piece_bit(r, a8);
                break;
        }
    }

    if (promoted) {
        board->pop_piece_bit(promoted, target);
    } else {
        board->pop_piece_bit(piece, target);
    }
    board->set_piece_bit(piece, source);

    if (undo->captured_piece >= 0) {
        board->set_piece_bit(undo->captured_piece, undo->captured_square);
    }

    board->set_en_passant(undo->en_passant);
    board->set_castle_rights(undo->castle_rights);
    board->set_half_move_clock(undo->half_move_clock);
    board->set_full_move_counter(undo->full_move_counter);
    board->set_hash_key(undo->hash_key);
    board->update_occupancies();
}

long long perft_driver(int depth, Board *board) {
    if (depth == 0) return 1;

    Moves move_list[1];
    generate_moves(board, move_list);

    long long nodes = 0;

    for (int i = 0; i < move_list->count; i++) {
        Undo undo;
        if (!make_move(board, move_list->moves[i], &undo, 0)) {
            continue;
        }

        nodes += perft_driver(depth - 1, board);

        undo_move(board, &undo);
    }

    return nodes;
}

long long uci_perft(Board *board, int depth) {
    if (depth == 0) {
        return 1;
    }

    Moves move_list[1];
    generate_moves(board, move_list);

    long long total = 0;

    for (int i = 0; i < move_list->count; i++) {
        Undo undo;
        if (!make_move(board, move_list->moves[i], &undo, 0)) {
            continue;
        }

        long long nodes = perft_driver(depth - 1, board);
        total += nodes;

        undo_move(board, &undo);

        char uci[6];
        Board::sq_to_chars(get_move_source(move_list->moves[i]), uci);
        Board::sq_to_chars(get_move_target(move_list->moves[i]), uci + 2);
        const int promo = get_move_promoted(move_list->moves[i]);
        if (promo) {
            char c = 'q';
            if (promo == R || promo == r) c = 'r';
            else if (promo == B || promo == b) c = 'b';
            else if (promo == N || promo == n) c = 'n';
            uci[4] = c;
            uci[5] = '\0';
        } else {
            uci[4] = '\0';
        }
        std::cout << "info string " << uci << " " << nodes << std::endl;
    }

    return total;
}
