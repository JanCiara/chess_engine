#pragma once
#include <string>
#include <unordered_map>
#include "defs.h"

class Board {
public:
    static constexpr char ascii_pieces[12] = {'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k'};

    inline static const std::unordered_map<char, int> ascii_to_int = {
        {'P', P},
        {'N', N},
        {'B', B},
        {'R', R},
        {'Q', Q},
        {'K', K},
        {'p', p},
        {'n', n},
        {'b', b},
        {'r', r},
        {'q', q},
        {'k', k}
    };

    Board();

    void init_start_position();
    void update_occupancies();
    static void print_bitboard(U64 bb);
    void print_board() const;
    void parseFEN(const std::string& fen);

    static int sq_to_int(const std::string& square);
    static std::string int_to_sq(int square);
    static void sq_to_chars(int square, char out[2]);

    int side() const { return side_; }
    void flip_side() { side_ ^= 1; }
    void set_side(int side) { side_ = side; }

    U64 hash_key() const { return hash_key_; }
    void set_hash_key(U64 key) { hash_key_ = key; }

    U64 bitboard(int piece) const { return bitboards_[piece]; }
    void pop_piece_bit(int piece, int square) {
        pop_bit(bitboards_[piece], square);
        if (mailbox_[square] == piece) {
            mailbox_[square] = -1;
        }
    }
    void set_piece_bit(int piece, int square) {
        set_bit(bitboards_[piece], square);
        mailbox_[square] = piece;
    }
    bool piece_on(int piece, int square) const { return get_bit(bitboards_[piece], square); }

    U64 occupancy(int index) const { return occupancies_[index]; }

    int en_passant() const { return en_passant_; }
    void set_en_passant(int square) { en_passant_ = square; }
    void clear_en_passant() { en_passant_ = -1; }

    int castle_rights() const { return castle_; }
    void set_castle_rights(int rights) { castle_ = rights; }
    void mask_castle_rights(int mask) { castle_ &= mask; }

    int half_move_clock() const { return half_move_clock_; }
    void set_half_move_clock(int clock) { half_move_clock_ = clock; }
    void increment_half_move_clock() { half_move_clock_++; }
    void reset_half_move_clock() { half_move_clock_ = 0; }

    int full_move_counter() const { return full_move_counter_; }
    void set_full_move_counter(int counter) { full_move_counter_ = counter; }
    void increment_full_move_counter() { full_move_counter_++; }

    int piece_at(int square) const;

    struct NullUndo {
        U64 hash_key = 0;
        int en_passant = -1;
    };
    void apply_null_move(NullUndo* undo);
    void undo_null_move(const NullUndo* undo);

private:
    U64 bitboards_[12] = {0ULL};
    int mailbox_[64];
    U64 occupancies_[3] = {0ULL};
    int side_ = 0;
    int en_passant_ = -1;
    int castle_ = 15;
    int full_move_counter_ = 0;
    int half_move_clock_ = 0;
    U64 hash_key_ = 0;
};
