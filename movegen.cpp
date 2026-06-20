#include "movegen.h"
#include "tt.h"

#include <ctime>
#include <iostream>

int is_square_attacked(int square, int side, const Board *board);


const int castling_rights[64] = {
    13, 15, 15, 15, 12, 15, 15, 14,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    7, 15, 15, 15, 3, 15, 15, 11
};

U64 bishop_attacks[64][512];
U64 rook_attacks[64][4096];

U64 bishop_masks[64];
U64 rook_masks[64];

U64 knight_attacks[64];
U64 king_attacks[64];
U64 pawn_attacks[2][64];

U64 noNoEa(U64 b) { return (b & notHFile) << 17; }
U64 noEaEa(U64 b) { return (b & notGHFile) << 10; }
U64 soEaEa(U64 b) { return (b & notGHFile) >> 6; }
U64 soSoEa(U64 b) { return (b & notHFile) >> 15; }
U64 noNoWe(U64 b) { return (b & notAFile) << 15; }
U64 noWeWe(U64 b) { return (b & notABFile) << 6; }
U64 soWeWe(U64 b) { return (b & notABFile) >> 10; }
U64 soSoWe(U64 b) { return (b & notAFile) >> 17; }

U64 noOne(U64 b) { return b << 8; }
U64 eaOne(U64 b) { return (b & notHFile) << 1; }
U64 weOne(U64 b) { return (b & notAFile) >> 1; }
U64 soOne(U64 b) { return b >> 8; }
U64 noEaOne(U64 b) { return (b & notHFile) << 9; }
U64 noWeOne(U64 b) { return (b & notAFile) << 7; }
U64 soWeOne(U64 b) { return (b & notAFile) >> 9; }
U64 soEaOne(U64 b) { return (b & notHFile) >> 7; }

static void add_ray_squares(U64 &attacks, int tr, int tf, int dr, int df,
                            int r_min, int r_max, int f_min, int f_max,
                            U64 block, bool stop_at_block) {
    for (int r = tr + dr, f = tf + df; r >= r_min && r <= r_max && f >= f_min && f <= f_max;
         r += dr, f += df) {
        set_bit(attacks, r * 8 + f);
        if (stop_at_block && get_bit(block, r * 8 + f)) break;
    }
}

static void add_slider_mask(U64 &attacks, int tr, int tf, int dr, int df) {
    int r_min = (dr < 0) ? 1 : 0;
    int r_max = (dr > 0) ? 6 : 7;
    int f_min = (df < 0) ? 1 : 0;
    int f_max = (df > 0) ? 6 : 7;
    add_ray_squares(attacks, tr, tf, dr, df, r_min, r_max, f_min, f_max, 0, false);
}

static void add_slider_attacks(U64 &attacks, int tr, int tf, int dr, int df, U64 block) {
    add_ray_squares(attacks, tr, tf, dr, df, 0, 7, 0, 7, block, true);
}

static U64 compute_slider_attacks(int square, U64 block,
                                  const int directions[][2], int direction_count,
                                  bool stop_at_block) {
    U64 attacks = 0ULL;
    int tr = square / 8;
    int tf = square % 8;

    for (int i = 0; i < direction_count; i++) {
        if (stop_at_block) {
            add_slider_attacks(attacks, tr, tf, directions[i][0], directions[i][1], block);
        } else {
            add_slider_mask(attacks, tr, tf, directions[i][0], directions[i][1]);
        }
    }
    return attacks;
}

U64 mask_bishop_attacks(int square) {
    static const int directions[4][2] = {{1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
    return compute_slider_attacks(square, 0, directions, 4, false);
}

U64 mask_rook_attacks(int square) {
    static const int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    return compute_slider_attacks(square, 0, directions, 4, false);
}

U64 bishop_attacks_on_the_fly(int square, U64 block) {
    static const int directions[4][2] = {{1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
    return compute_slider_attacks(square, block, directions, 4, true);
}

U64 rook_attacks_on_the_fly(int square, U64 block) {
    static const int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    return compute_slider_attacks(square, block, directions, 4, true);
}

U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask) {
    U64 occupancy = 0ULL;
    for (int count = 0; count < bits_in_mask; count++) {
        int square = get_LSB(attack_mask);
        pop_bit(attack_mask, square);

        if (index & (1 << count)) {
            set_bit(occupancy, square);
        }
    }
    return occupancy;
}

const U64 rook_magic_numbers[64] = {
    0x8a80104000800020ULL,
    0x140002000100040ULL,
    0x2801880a0017001ULL,
    0x100081001000420ULL,
    0x200020010080420ULL,
    0x3001c0002010008ULL,
    0x8480008002000100ULL,
    0x2080088004402900ULL,
    0x800098204000ULL,
    0x2024401000200040ULL,
    0x100802000801000ULL,
    0x120800800801000ULL,
    0x208808088000400ULL,
    0x2802200800400ULL,
    0x2200800100020080ULL,
    0x801000060821100ULL,
    0x80044006422000ULL,
    0x100808020004000ULL,
    0x12108a0010204200ULL,
    0x140848010000802ULL,
    0x481828014002800ULL,
    0x8094004002004100ULL,
    0x4010040010010802ULL,
    0x20008806104ULL,
    0x100400080208000ULL,
    0x2040002120081000ULL,
    0x21200680100081ULL,
    0x20100080080080ULL,
    0x2000a00200410ULL,
    0x20080800400ULL,
    0x80088400100102ULL,
    0x80004600042881ULL,
    0x4040008040800020ULL,
    0x440003000200801ULL,
    0x4200011004500ULL,
    0x188020010100100ULL,
    0x14800401802800ULL,
    0x2080040080800200ULL,
    0x124080204001001ULL,
    0x200046502000484ULL,
    0x480400080088020ULL,
    0x1000422010034000ULL,
    0x30200100110040ULL,
    0x100021010009ULL,
    0x2002080100110004ULL,
    0x202008004008002ULL,
    0x20020004010100ULL,
    0x2048440040820001ULL,
    0x101002200408200ULL,
    0x40802000401080ULL,
    0x4008142004410100ULL,
    0x2060820c0120200ULL,
    0x1001004080100ULL,
    0x20c020080040080ULL,
    0x2935610830022400ULL,
    0x44440041009200ULL,
    0x280001040802101ULL,
    0x2100190040002085ULL,
    0x80c0084100102001ULL,
    0x4024081001000421ULL,
    0x20030a0244872ULL,
    0x12001008414402ULL,
    0x2006104900a0804ULL,
    0x1004081002402ULL,
};

const U64 bishop_magic_numbers[64] = {
    0x40040844404084ULL,
    0x2004208a004208ULL,
    0x10190041080202ULL,
    0x108060845042010ULL,
    0x581104180800210ULL,
    0x2112080446200010ULL,
    0x1080820820060210ULL,
    0x3c0808410220200ULL,
    0x4050404440404ULL,
    0x21001420088ULL,
    0x24d0080801082102ULL,
    0x1020a0a020400ULL,
    0x40308200402ULL,
    0x4011002100800ULL,
    0x401484104104005ULL,
    0x801010402020200ULL,
    0x400210c3880100ULL,
    0x404022024108200ULL,
    0x810018200204102ULL,
    0x4002801a02003ULL,
    0x85040820080400ULL,
    0x810102c808880400ULL,
    0xe900410884800ULL,
    0x8002020480840102ULL,
    0x220200865090201ULL,
    0x2010100a02021202ULL,
    0x152048408022401ULL,
    0x20080002081110ULL,
    0x4001001021004000ULL,
    0x800040400a011002ULL,
    0xe4004081011002ULL,
    0x1c004001012080ULL,
    0x8004200962a00220ULL,
    0x8422100208500202ULL,
    0x2000402200300c08ULL,
    0x8646020080080080ULL,
    0x80020a0200100808ULL,
    0x2010004880111000ULL,
    0x623000a080011400ULL,
    0x42008c0340209202ULL,
    0x209188240001000ULL,
    0x400408a884001800ULL,
    0x110400a6080400ULL,
    0x1840060a44020800ULL,
    0x90080104000041ULL,
    0x201011000808101ULL,
    0x1a2208080504f080ULL,
    0x8012020600211212ULL,
    0x500861011240000ULL,
    0x180806108200800ULL,
    0x4000020e01040044ULL,
    0x300000261044000aULL,
    0x802241102020002ULL,
    0x20906061210001ULL,
    0x5a84841004010310ULL,
    0x4010801011c04ULL,
    0xa010109502200ULL,
    0x4a02012000ULL,
    0x500201010098b028ULL,
    0x8040002811040900ULL,
    0x28000010020204ULL,
    0x6000020202d0240ULL,
    0x8918844842082200ULL,
    0x4010011029020020ULL,
};

const int rook_relevant_bits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12
};

const int bishop_relevant_bits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6
};

template <int MaxEntries>
static void init_one_slider(U64 masks[64], U64 attacks[64][MaxEntries],
                            U64 (*mask_fn)(int), U64 (*attacks_fn)(int, U64),
                            const U64 magic_numbers[64], const int relevant_bits[64]) {
    for (int square = 0; square < 64; square++) {
        masks[square] = mask_fn(square);
        U64 attack_mask = masks[square];
        int relevant_bits_count = count_bits(attack_mask);
        int occupancy_indices = 1 << relevant_bits_count;

        for (int index = 0; index < occupancy_indices; index++) {
            U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
            int magic_index = (occupancy * magic_numbers[square]) >> (64 - relevant_bits[square]);
            attacks[square][magic_index] = attacks_fn(square, occupancy);
        }
    }
}

void init_sliders_attacks(int is_bishop) {
    if (is_bishop) {
        init_one_slider(bishop_masks, bishop_attacks, mask_bishop_attacks, bishop_attacks_on_the_fly,
                        bishop_magic_numbers, bishop_relevant_bits);
    } else {
        init_one_slider(rook_masks, rook_attacks, mask_rook_attacks, rook_attacks_on_the_fly,
                        rook_magic_numbers, rook_relevant_bits);
    }
}

void init_all_attacks() {
    init_sliders_attacks(1);
    init_sliders_attacks(0);

    for (int sq = 0; sq < 64; sq++) {
        U64 bb = 1ULL << sq;
        pawn_attacks[WHITE][sq] = noEaOne(bb) | noWeOne(bb);
        pawn_attacks[BLACK][sq] = soEaOne(bb) | soWeOne(bb);

        knight_attacks[sq] = noNoEa(bb) | noEaEa(bb) |
                             soEaEa(bb) | soSoEa(bb) |
                             noNoWe(bb) | noWeWe(bb) |
                             soWeWe(bb) | soSoWe(bb);

        king_attacks[sq] = noOne(bb) | eaOne(bb) | weOne(bb) | soOne(bb) |
                           noEaOne(bb) | noWeOne(bb) |
                           soEaOne(bb) | soWeOne(bb);
    }
}

template <int MaxEntries>
static U64 get_slider_attacks(int square, U64 occupancy, U64 masks[64],
                              const U64 magic_numbers[64], const int relevant_bits[64],
                              U64 attacks[64][MaxEntries]) {
    occupancy &= masks[square];
    occupancy *= magic_numbers[square];
    occupancy >>= 64 - relevant_bits[square];
    return attacks[square][occupancy];
}

U64 get_bishop_attacks(int square, U64 occupancy) {
    return get_slider_attacks(square, occupancy, bishop_masks, bishop_magic_numbers,
                              bishop_relevant_bits, bishop_attacks);
}

U64 get_rook_attacks(int square, U64 occupancy) {
    return get_slider_attacks(square, occupancy, rook_masks, rook_magic_numbers,
                              rook_relevant_bits, rook_attacks);
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

// Check if a square is attacked by a given side
int is_square_attacked(int square, int side, const Board *board) {
    // 1. Pawns
    if ((side == WHITE) && (pawn_attacks[BLACK][square] & board->bitboard(P))) return 1;
    if ((side == BLACK) && (pawn_attacks[WHITE][square] & board->bitboard(p))) return 1;

    // 2. Knights
    if (knight_attacks[square] & ((side == WHITE) ? board->bitboard(N) : board->bitboard(n))) return 1;

    // 3. King
    if (king_attacks[square] & ((side == WHITE) ? board->bitboard(K) : board->bitboard(k))) return 1;

    // 4. Bishops & Queens
    U64 bishop_targets = get_bishop_attacks(square, board->occupancy(2));
    U64 diagonal_attackers = (side == WHITE)
                                 ? (board->bitboard(B) | board->bitboard(Q))
                                 : (board->bitboard(b) | board->bitboard(q));
    if (bishop_targets & diagonal_attackers) return 1;

    // 5. Rooks & Queens
    U64 rook_targets = get_rook_attacks(square, board->occupancy(2));
    U64 straight_attackers = (side == WHITE)
                                 ? (board->bitboard(R) | board->bitboard(Q))
                                 : (board->bitboard(r) | board->bitboard(q));
    if (rook_targets & straight_attackers) return 1;

    return 0;
}

// Main move generation function
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

            // Mask off friendly pieces
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

// Returns 1 if move is legal, 0 if illegal (king left in check)
int make_move(Board *board, int move, int capture_only) {
    // 1. ALL MOVES vs CAPTURES ONLY
    if (capture_only) {
        // If we only want captures, return 0 for quiet moves
        if (!get_move_capture(move)) return 0;
    }

    // 2. BACKUP BOARD STATE
    // We copy the whole structure. If the move turns out to be illegal (check), we restore it.
    Board board_copy = *board;

    // 3. PARSE MOVE INFO
    int source_square = get_move_source(move);
    int target_square = get_move_target(move);
    int piece = get_move_piece(move);
    int promoted_piece = get_move_promoted(move);
    int capture = get_move_capture(move);
    int double_push = get_move_double(move);
    int enpassant = get_move_enpassant(move);
    int castling = get_move_castling(move);

    // 4. MOVE THE PIECE
    // Remove from source, put on target
    board->pop_piece_bit(piece, source_square);
    board->set_piece_bit(piece, target_square);

    // 5. HANDLE CAPTURES
    if (capture) {
        // Identify which opponent piece was on target_square and remove it
        // Range: If white moves (0..5), opponent is black (6..11) and vice versa
        int start_piece, end_piece;

        if (board->side() == WHITE) {
            start_piece = p;
            end_piece = k;
        } else {
            start_piece = P;
            end_piece = K;
        }

        for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++) {
            if (board->piece_on(bb_piece, target_square)) {
                board->pop_piece_bit(bb_piece, target_square);
                break; // Found and removed the victim
            }
        }
    }

    // 6. HANDLE PROMOTION
    if (promoted_piece) {
        board->pop_piece_bit((board->side() == WHITE) ? P : p, target_square);
        board->set_piece_bit(promoted_piece, target_square);
    }

    // 7. HANDLE EN PASSANT
    if (enpassant) {
        if (board->side() == WHITE) {
            board->pop_piece_bit(p, target_square - 8);
        } else {
            board->pop_piece_bit(P, target_square + 8);
        }
    }

    // 8. RESET EN PASSANT SQUARE
    board->clear_en_passant();

    // 9. HANDLE DOUBLE PUSH (Set new en passant square)
    if (double_push) {
        if (board->side() == WHITE) {
            board->set_en_passant(source_square + 8);
        } else {
            board->set_en_passant(source_square - 8);
        }
    }

    // 10. HANDLE CASTLING
    if (castling) {
        switch (target_square) {
            // White Short (g1) -> Move Rook h1-f1
            case g1:
                board->pop_piece_bit(R, h1);
                board->set_piece_bit(R, f1);
                break;
            // White Long (c1) -> Move Rook a1-d1
            case c1:
                board->pop_piece_bit(R, a1);
                board->set_piece_bit(R, d1);
                break;
            // Black Short (g8) -> Move Rook h8-f8
            case g8:
                board->pop_piece_bit(r, h8);
                board->set_piece_bit(r, f8);
                break;
            // Black Long (c8) -> Move Rook a8-d8
            case c8:
                board->pop_piece_bit(r, a8);
                board->set_piece_bit(r, d8);
                break;
        }
    }

    // 11. UPDATE CASTLING RIGHTS
    board->mask_castle_rights(castling_rights[source_square]);
    board->mask_castle_rights(castling_rights[target_square]);

    // 12. UPDATE OCCUPANCIES
    board->update_occupancies();

    if (capture || piece == P || piece == p || promoted_piece) {
        board->reset_half_move_clock();
    } else {
        board->increment_half_move_clock();
    }

    if (board->side() == BLACK) {
        board->increment_full_move_counter();
    }

    // 13. CHANGE SIDE
    board->flip_side();

    // 14. LEGALITY CHECK

    int side_moved = board->side() ^ 1;

    int king_bitboard_index = (side_moved == WHITE) ? K : k;
    int king_square = get_LSB(board->bitboard(king_bitboard_index));

    if (is_square_attacked(king_square, board->side(), board)) {
        *board = board_copy;
        return 0;
    }

    board->set_hash_key(compute_hash(board));
    return 1;
}

long long perft_driver(int depth, Board *board) {
    if (depth == 0) return 1;

    Moves move_list[1];
    generate_moves(board, move_list);

    long long nodes = 0;

    for (int i = 0; i < move_list->count; i++) {
        Board copy = *board;

        // Try to make move
        if (!make_move(board, move_list->moves[i], 0)) {
            continue;
        }

        // Recursive call
        nodes += perft_driver(depth - 1, board);

        // Restore board state (undo)
        *board = copy;
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
        Board copy = *board;

        if (!make_move(board, move_list->moves[i], 0)) {
            continue;
        }

        long long nodes = perft_driver(depth - 1, board);
        total += nodes;

        *board = copy;

        std::cout << "info string "
                  << Board::int_to_sq(get_move_source(move_list->moves[i]))
                  << Board::int_to_sq(get_move_target(move_list->moves[i]))
                  << " " << nodes << std::endl;
    }

    return total;
}
