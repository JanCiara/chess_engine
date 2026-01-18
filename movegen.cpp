#include "movegen.h"

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

U64 mask_bishop_attacks(int square) {
    U64 attacks = 0ULL;
    int r, f;
    int tr = square / 8;
    int tf = square % 8;

    for (r = tr + 1, f = tf + 1; r < 7 && f < 7; r++, f++)
        set_bit(attacks, r * 8 + f);
    for (r = tr - 1, f = tf + 1; r > 0 && f < 7; r--, f++)
        set_bit(attacks, r * 8 + f);
    for (r = tr + 1, f = tf - 1; r < 7 && f > 0; r++, f--)
        set_bit(attacks, r * 8 + f);
    for (r = tr - 1, f = tf - 1; r > 0 && f > 0; r--, f--)
        set_bit(attacks, r * 8 + f);

    return attacks;
}

U64 mask_rook_attacks(int square) {
    U64 attacks = 0ULL;
    int r, f;
    int tr = square / 8;
    int tf = square % 8;

    for (r = tr + 1; r < 7; r++)
        set_bit(attacks, r * 8 + tf);
    for (r = tr - 1; r > 0; r--)
        set_bit(attacks, r * 8 + tf);
    for (f = tf + 1; f < 7; f++)
        set_bit(attacks, tr * 8 + f);
    for (f = tf - 1; f > 0; f--)
        set_bit(attacks, tr * 8 + f);

    return attacks;
}

U64 bishop_attacks_on_the_fly(int square, U64 block) {
    U64 attacks = 0ULL;
    int r, f;
    int tr = square / 8;
    int tf = square % 8;

    for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++) {
        set_bit(attacks, r * 8 + f);
        if (get_bit(block, r * 8 + f)) break;
    }
    for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++) {
        set_bit(attacks, r * 8 + f);
        if (get_bit(block, r * 8 + f)) break;
    }
    for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--) {
        set_bit(attacks, r * 8 + f);
        if (get_bit(block, r * 8 + f)) break;
    }
    for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--) {
        set_bit(attacks, r * 8 + f);
        if (get_bit(block, r * 8 + f)) break;
    }
    return attacks;
}

U64 rook_attacks_on_the_fly(int square, U64 block) {
    U64 attacks = 0ULL;
    int r, f;
    int tr = square / 8;
    int tf = square % 8;

    for (r = tr + 1; r <= 7; r++) {
        set_bit(attacks, r * 8 + tf);
        if (get_bit(block, r * 8 + tf)) break;
    }
    for (r = tr - 1; r >= 0; r--) {
        set_bit(attacks, r * 8 + tf);
        if (get_bit(block, r * 8 + tf)) break;
    }
    for (f = tf + 1; f <= 7; f++) {
        set_bit(attacks, tr * 8 + f);
        if (get_bit(block, tr * 8 + f)) break;
    }
    for (f = tf - 1; f >= 0; f--) {
        set_bit(attacks, tr * 8 + f);
        if (get_bit(block, tr * 8 + f)) break;
    }
    return attacks;
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

void init_sliders_attacks(int is_bishop) {
    for (int square = 0; square < 64; square++) {
        bishop_masks[square] = mask_bishop_attacks(square);
        rook_masks[square] = mask_rook_attacks(square);

        U64 attack_mask = is_bishop ? bishop_masks[square] : rook_masks[square];
        int relevant_bits_count = count_bits(attack_mask);

        int occupancy_indices = 1 << relevant_bits_count;

        for (int index = 0; index < occupancy_indices; index++) {
            U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);

            int magic_index;
            if (is_bishop) {
                magic_index = (occupancy * bishop_magic_numbers[square]) >> (64 - bishop_relevant_bits[square]);
                bishop_attacks[square][magic_index] = bishop_attacks_on_the_fly(square, occupancy);
            } else {
                magic_index = (occupancy * rook_magic_numbers[square]) >> (64 - rook_relevant_bits[square]);
                rook_attacks[square][magic_index] = rook_attacks_on_the_fly(square, occupancy);
            }
        }
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

U64 get_bishop_attacks(int square, U64 occupancy) {
    occupancy &= bishop_masks[square];
    occupancy *= bishop_magic_numbers[square];
    occupancy >>= 64 - bishop_relevant_bits[square];
    return bishop_attacks[square][occupancy];
}

U64 get_rook_attacks(int square, U64 occupancy) {
    occupancy &= rook_masks[square];
    occupancy *= rook_magic_numbers[square];
    occupancy >>= 64 - rook_relevant_bits[square];
    return rook_attacks[square][occupancy];
}

U64 get_queen_attacks(int square, U64 occupancy) {
    return get_bishop_attacks(square, occupancy) | get_rook_attacks(square, occupancy);
}

static void add_move(Moves *move_list, int move) {
    move_list->moves[move_list->count] = move;
    move_list->count++;
}

void generate_moves(const Board *board, Moves *move_list) {
    move_list->count = 0;

    int source_square, target_square;
    int side = board->side;

    U64 bitboard, attacks;
    U64 occupancy = board->occupancies[2];
    U64 enemy_occupancy = (side == WHITE) ? board->occupancies[BLACK] : board->occupancies[WHITE];

    if (side == WHITE) {
        // --- WHITE PAWNS ---
        bitboard = board->bitboards[P];

        while (bitboard) {
            source_square = get_LSB(bitboard);

            // Quiet moves
            target_square = source_square + 8;

            if (!(target_square > h8) && !get_bit(occupancy, target_square)) {
                // Promotion
                if (source_square >= a7 && source_square <= h7) {
                    add_move(move_list, encode_move(source_square, target_square, P, Q, 0, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, P, R, 0, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, P, B, 0, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, P, N, 0, 0, 0, 0));
                } else {
                    // Single push
                    add_move(move_list, encode_move(source_square, target_square, P, 0, 0, 0, 0, 0));

                    // Double push
                    if ((source_square >= a2 && source_square <= h2) && !get_bit(occupancy, source_square + 16)) {
                        add_move(move_list, encode_move(source_square, source_square + 16, P, 0, 0, 1, 0, 0));
                    }
                }
            }

            // Captures
            attacks = pawn_attacks[WHITE][source_square];
            U64 en_passant_bit = (board->en_passant != -1) ? (1ULL << board->en_passant) : 0ULL;
            U64 valid_captures = attacks & (enemy_occupancy | en_passant_bit);

            while (valid_captures) {
                target_square = get_LSB(valid_captures);
                int en_passant_flag = (target_square == board->en_passant) ? 1 : 0;

                // Capture promotion
                if (source_square >= a7 && source_square <= h7) {
                    add_move(move_list, encode_move(source_square, target_square, P, Q, 1, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, P, R, 1, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, P, B, 1, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, P, N, 1, 0, 0, 0));
                } else {
                    add_move(move_list, encode_move(source_square, target_square, P, 0, 1, 0, en_passant_flag, 0));
                }
                pop_bit(valid_captures, target_square);
            }

            pop_bit(bitboard, source_square);
        }
    } else {
        // --- BLACK PAWNS ---
        bitboard = board->bitboards[p];

        while (bitboard) {
            source_square = get_LSB(bitboard);

            // Quiet moves
            target_square = source_square - 8;

            if (!(target_square < a1) && !get_bit(occupancy, target_square)) {
                // Promotion
                if (source_square >= a2 && source_square <= h2) {
                    add_move(move_list, encode_move(source_square, target_square, p, q, 0, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, p, r, 0, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, p, b, 0, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, p, n, 0, 0, 0, 0));
                } else {
                    // Single push
                    add_move(move_list, encode_move(source_square, target_square, p, 0, 0, 0, 0, 0));

                    // Double push
                    if ((source_square >= a7 && source_square <= h7) && !get_bit(occupancy, source_square - 16)) {
                        add_move(move_list, encode_move(source_square, source_square - 16, p, 0, 0, 1, 0, 0));
                    }
                }
            }

            // Captures
            attacks = pawn_attacks[BLACK][source_square];
            U64 en_passant_bit = (board->en_passant != -1) ? (1ULL << board->en_passant) : 0ULL;
            U64 valid_captures = attacks & (enemy_occupancy | en_passant_bit);

            while (valid_captures) {
                target_square = get_LSB(valid_captures);
                int en_passant_flag = (target_square == board->en_passant) ? 1 : 0;

                // Capture promotion
                if (source_square >= a2 && source_square <= h2) {
                    add_move(move_list, encode_move(source_square, target_square, p, q, 1, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, p, r, 1, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, p, b, 1, 0, 0, 0));
                    add_move(move_list, encode_move(source_square, target_square, p, n, 1, 0, 0, 0));
                } else {
                    add_move(move_list, encode_move(source_square, target_square, p, 0, 1, 0, en_passant_flag, 0));
                }
                pop_bit(valid_captures, target_square);
            }

            pop_bit(bitboard, source_square);
        }
    }
}
