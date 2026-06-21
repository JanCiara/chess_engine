#include "board.h"
#include <iostream>
#include <cstring>
#include <sstream>

#include "movegen.h"
#include "tt.h"

Board::Board() {
    std::memset(mailbox_, -1, sizeof(mailbox_));
    init_start_position();
    update_occupancies();
    hash_key_ = compute_hash(this);
}

int Board::piece_at(int square) const {
    return mailbox_[square];
}

void Board::sq_to_chars(int square, char out[2]) {
    out[0] = static_cast<char>('a' + (square % 8));
    out[1] = static_cast<char>('1' + (square / 8));
}

void Board::apply_null_move(NullUndo* undo) {
    undo->hash_key = hash_key_;
    undo->en_passant = en_passant_;

    U64 key = hash_key_;
    key ^= zobrist_side_key();
    if (en_passant_ != -1) {
        key ^= zobrist_ep_key(en_passant_);
        key ^= zobrist_ep_key(64);
    }

    flip_side();
    clear_en_passant();
    hash_key_ = key;
}

void Board::undo_null_move(const NullUndo* undo) {
    flip_side();
    en_passant_ = undo->en_passant;
    hash_key_ = undo->hash_key;
}

int Board::sq_to_int(const std::string &square) {
    const int file = square[0] - 'a'; // 'e' - 'a' = 4
    const int rank = square[1] - '1'; // '4' - '1' = 3

    return rank * 8 + file; // 3 * 8 + 4 = 28
}

std::string Board::int_to_sq(int square) {
    char sq[2];
    sq_to_chars(square, sq);
    return std::string(sq, 2);
}

void Board::init_start_position() {
    // White pawns
    for (int sq = a2; sq <= h2; sq++) {
        set_piece_bit(P, sq);
    }

    // White pieces
    set_piece_bit(R, a1);
    set_piece_bit(R, h1);
    set_piece_bit(N, b1);
    set_piece_bit(N, g1);
    set_piece_bit(B, c1);
    set_piece_bit(B, f1);
    set_piece_bit(Q, d1);
    set_piece_bit(K, e1);

    // Black pawns
    for (int sq = a7; sq <= h7; sq++) {
        set_piece_bit(p, sq);
    }

    // Black pieces
    set_piece_bit(r, a8);
    set_piece_bit(r, h8);
    set_piece_bit(n, b8);
    set_piece_bit(n, g8);
    set_piece_bit(b, c8);
    set_piece_bit(b, f8);
    set_piece_bit(q, d8);
    set_piece_bit(k, e8);
}

void Board::update_occupancies() {
    occupancies_[0] = 0ULL;
    occupancies_[1] = 0ULL;
    occupancies_[2] = 0ULL;

    for (int i = P; i <= K; i++) {
        occupancies_[0] |= bitboards_[i];
    }

    for (int i = p; i <= k; i++) {
        occupancies_[1] |= bitboards_[i];
    }

    occupancies_[2] = occupancies_[0] | occupancies_[1];
}

void Board::print_bitboard(const U64 bb) {
    std::cout << '\n';
    for (int rank = 7; rank >= 0; rank--) {
        // Wypisz numer rzędu
        std::cout << " " << rank + 1 << "  ";

        for (int file = 0; file < 8; file++) {
            int square = (rank * 8) + file; // Indeks pola (0..63)

            if (get_bit(bb, square))
                std::cout << " 1 ";
            else
                std::cout << " . ";
        }
        std::cout << '\n';
    }

    // Wypisz litery kolumn
    std::cout << "\n     a  b  c  d  e  f  g  h\n";
    std::cout << "\n   Bitboard Value: " << bb << "\n\n";
}

// Fancy print
void Board::print_board() const {
    std::cout << "\n";
    for (int rank = 7; rank >= 0; rank--) {
        std::cout << " " << rank + 1 << "  ";
        for (int file = 0; file < 8; file++) {
            int square = (rank * 8) + file;
            int piece = -1;

            piece = piece_at(square);

            if (piece != -1)
                std::cout << " " << ascii_pieces[piece] << " ";
            else
                std::cout << " . ";
        }
        std::cout << "\n";
    }
    std::cout << "\n     a  b  c  d  e  f  g  h\n\n";

    // Debug info
    std::cout << "     Side: " << (side_ == WHITE ? "White" : "Black") << "\n";
    std::cout << "     EnPassant: " << ((en_passant_ == -1) ? "-" : int_to_sq(en_passant_)) << "\n";
    std::cout << "     Castling: " << ((castle_ & WK) ? 'K' : '-') << ((castle_ & WQ) ? 'Q' : '-')
            << ((castle_ & BK) ? 'k' : '-') << ((castle_ & BQ) ? 'q' : '-') << "\n";
    std::cout << "     Timers: " << "Halfmove counter: " << half_move_clock_ << "\n"
            << "             Fullmove counter: " << full_move_counter_ << "\n\n";
}


void Board::parseFEN(const std::string& fen) {
    memset(bitboards_, 0, sizeof(bitboards_));
    memset(mailbox_, -1, sizeof(mailbox_));
    memset(occupancies_, 0, sizeof(occupancies_));


    const auto space_idx = fen.find(' ');
    int i = 0;
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int square = (rank * 8) + file;
            if (fen[i] >= '1' and fen[i] <= '8') {
                file += (fen[i] - '0') - 1;
                i++;
                continue;
            }
            if (fen[i] == '/') {
                i++;
                file--;
                continue;
            }

            if (auto it = ascii_to_int.find(fen[i]); it != ascii_to_int.end()) {
                int cur = it->second;
                set_piece_bit(cur, square);
                i++;
            } else {
                std::cout << "Found invalid char in FEN: " << fen[i] << '\n';
                return;
            }
        }
    }

    std::stringstream ss(fen.substr(space_idx + 1));
    std::string turn_tok, castle_tok, en_pas_tok;

    ss >> turn_tok;
    set_side(turn_tok == "w" ? WHITE : BLACK);

    set_castle_rights(0);
    ss >> castle_tok;
    for (const char c : castle_tok) {
        switch (c) {
            case 'K': castle_ |= WK; break;
            case 'Q': castle_ |= WQ; break;
            case 'k': castle_ |= BK; break;
            case 'q': castle_ |= BQ; break;
            default: break;
        }
    }

    ss >> en_pas_tok;
    set_en_passant((en_pas_tok == "-") ? -1 : sq_to_int(en_pas_tok));

    set_half_move_clock(0);
    set_full_move_counter(1);
    ss >> half_move_clock_;
    ss >> full_move_counter_;

    update_occupancies();
    set_hash_key(compute_hash(this));
}
