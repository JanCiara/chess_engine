#include "board.h"
#include <iostream>
#include <cstring>

#include "movegen.h"

Board::Board() {
    init_start_position();
    update_occupancies();
}

int Board::sq_to_int(const std::string &square) {
    const int file = square[0] - 'a'; // 'e' - 'a' = 4
    const int rank = square[1] - '1'; // '4' - '1' = 3

    return rank * 8 + file; // 3 * 8 + 4 = 28
}

std::string Board::int_to_sq(int square) {
    std::string s;

    const int file = square % 8;
    const int rank = square / 8;

    s += ('a' + file);
    s += ('1' + rank);

    return s;
}

void Board::init_start_position() {
    // White pawns
    for (int sq = a2; sq <= h2; sq++) {
        set_bit(bitboards[P], sq);
    }

    // White pieces
    set_bit(bitboards[R], a1);
    set_bit(bitboards[R], h1);
    set_bit(bitboards[N], b1);
    set_bit(bitboards[N], g1);
    set_bit(bitboards[B], c1);
    set_bit(bitboards[B], f1);
    set_bit(bitboards[Q], d1);
    set_bit(bitboards[K], e1);

    // Black pawns
    for (int sq = a7; sq <= h7; sq++) {
        set_bit(bitboards[p], sq);
    }

    // Black pieces
    set_bit(bitboards[r], a8);
    set_bit(bitboards[r], h8);
    set_bit(bitboards[n], b8);
    set_bit(bitboards[n], g8);
    set_bit(bitboards[b], c8);
    set_bit(bitboards[b], f8);
    set_bit(bitboards[q], d8);
    set_bit(bitboards[k], e8);
}

void Board::update_occupancies() {
    occupancies[0] = 0ULL;
    occupancies[1] = 0ULL;
    occupancies[2] = 0ULL;

    // White
    for (int i = P; i <= K; i++) occupancies[0] |= bitboards[i];

    // Black
    for (int i = p; i <= k; i++) occupancies[1] |= bitboards[i];

    occupancies[2] = occupancies[0] | occupancies[1];
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

            for (int bb_piece = P; bb_piece <= k; bb_piece++) {
                if (get_bit(bitboards[bb_piece], square)) {
                    piece = bb_piece;
                    break;
                }
            }

            if (piece != -1)
                std::cout << " " << ascii_pieces[piece] << " ";
            else
                std::cout << " . ";
        }
        std::cout << "\n";
    }
    std::cout << "\n     a  b  c  d  e  f  g  h\n\n";

    // Debug info
    std::cout << "     Side: " << (side == WHITE ? "White" : "Black") << "\n";
    std::cout << "     EnPassant: " << ((en_passant == -1) ? "-" : int_to_sq(en_passant)) << "\n";
    std::cout << "     Castling: " << ((castle & WK) ? 'K' : '-') << ((castle & WQ) ? 'Q' : '-')
            << ((castle & BK) ? 'k' : '-') << ((castle & BQ) ? 'q' : '-') << "\n";
    std::cout << "     Timers: " << "Halfmove counter: " << half_move_clock << "\n"
            << "             Fullmove counter: " << full_move_counter << "\n\n";
}


void Board::parseFEN(const std::string &fen) {
    memset(bitboards, 0, sizeof(bitboards));
    memset(occupancies, 0, sizeof(occupancies));


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
                set_bit(bitboards[cur], square);
                i++;
            } else {
                std::cout << "Found invalid char in FEN: " << fen[i] << '\n';
                return;
            }
        }
    }

    auto cur_idx = space_idx + 1;
    const char turn = fen[cur_idx];
    side = turn == 'w' ? WHITE : BLACK;
    cur_idx += 2;

    while (fen[cur_idx] != ' ') {
        switch (fen[cur_idx]) {
            case '-':
                castle = 0;
                break;
            case 'K':
                castle |= WK;
                break;
            case 'Q':
                castle |= WQ;
                break;
            case 'k':
                castle |= BK;
                break;
            case 'q':
                castle |= BQ;
                break;
            default:
                break;
        }
        cur_idx++;
    }
    cur_idx++;

    std::string en_pas;
    if (fen[cur_idx] == '-') {
        en_pas = '-';
        en_passant = -1;
        cur_idx += 2;
    } else {
        en_pas = fen.substr(cur_idx, 2);
        en_passant = sq_to_int(en_pas);
        cur_idx++;
    }


    if (fen[cur_idx + 1] == ' ') {
        half_move_clock = fen[cur_idx] - '0';
        cur_idx += 2;
    } else {
        const std::string tmp = fen.substr(cur_idx, 2);
        half_move_clock = (tmp[0] - '0') * 10 + tmp[1] - '0';
        cur_idx += 3;
    }

    if (cur_idx + 1 < fen.size()) {
        const std::string tmp = fen.substr(cur_idx, 2);
        full_move_counter = ((tmp[0] - '0') * 10) + tmp[1] - '0';
    } else {
        full_move_counter = fen[cur_idx] - '0';
    }

    update_occupancies();
}
