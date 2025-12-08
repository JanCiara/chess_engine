#include <iostream>
#include <vector>
#include <cstdint>
#include <string>
#include <cstring>
#include <unordered_map>
#include "defs.h"


class Board {
public:
    // 6 White, 6 Black
    U64 bitboards[12];

    // [0] - White, [1] - Black, [2] - All
    U64 occupancies[3];

    // 0 - White, 1 - Black
    int side = 0;

    // Represents the field (e.g. d7) where the en passant is possible
    int en_passant = -1; // -1 means no en passant possible

    // Bits representing the ability to castle
    // 1 (0001) - White king (WK) can short castle
    // 2 (0010) - White king (WQ) can long castle
    // 4 (0100) - White king (BK) can short castle
    // 8 (1000) - White king (BQ) can long castle
    int castle = 15;

    int full_move_counter = 0;
    int half_move_clock = 0;

    const char ascii_pieces[12] = {'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k'};

    Board() {
        for (int i = 0; i < 12; i++) bitboards[i] = 0ULL;
        for (int i = 0; i < 3; i++) occupancies[i] = 0ULL;

        init_start_position();
        update_occupancies();
    }

    int sq_to_int(const std::string &square) {
        // square[0] to litera (np. 'e'), square[1] to cyfra (np. '4')

        int file = square[0] - 'a'; // 'e' - 'a' = 4
        int rank = square[1] - '1'; // '4' - '1' = 3

        return rank * 8 + file; // 3 * 8 + 4 = 28
    }

    std::string int_to_sq(int square) {
        std::string s = "";

        int file = square % 8;
        int rank = square / 8;

        s += ('a' + file); // rzutowanie int na char
        s += ('1' + rank);

        return s;
    }

    void init_start_position() {
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

    void update_occupancies() {
        occupancies[0] = 0ULL;
        occupancies[1] = 0ULL;
        occupancies[2] = 0ULL;

        // White
        for (int i = P; i <= K; i++) occupancies[0] |= bitboards[i];

        // Black
        for (int i = p; i <= k; i++) occupancies[1] |= bitboards[i];

        occupancies[2] = occupancies[0] | occupancies[1];
    }

    void print_bitboard(U64 bb) {
        std::cout << '\n';
        for (int rank = 7; rank >= 0; rank--) {
            for (int file = 0; file < 8; file++) {
                int square = (rank * 8) + file;
                if (get_bit(bb, square)) std::cout << " 1 ";
                else std::cout << " . ";
            }
            std::cout << '\n';
        }
        std::cout << "\n   Bitboard Value: " << bb << "\n\n";
    }

    // Fancy print
    void print_board() {
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
        std::cout << "     Timers: "<< "Halfmove counter: " << half_move_clock << "\n"
                        << "             Fullmove counter: " << full_move_counter << "\n\n";
    }

    const std::unordered_map<char, int> ascii_to_int = {
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

    void parseFEN(const std::string &fen) {
        memset(bitboards, 0, sizeof(bitboards));
        memset(occupancies, 0, sizeof(occupancies));


        auto space_idx = fen.find(' ');
        int square;
        int i = 0;
        char c;
        for (int rank = 7; rank >= 0; rank--) {
            for (int file = 0; file < 8; file++) {
                char c = fen[i];
                square = (rank * 8) + file;
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

                auto it = ascii_to_int.find(fen[i]);
                if (it != ascii_to_int.end()) {
                    int cur = it->second;
                    set_bit(bitboards[cur], square);
                    i++;
                } else {
                    std::cout << "Found invalid char in FEN: " << fen[i] << '\n';
                    break;
                }
            }
        }

        auto cur_idx = space_idx + 1;
        char turn = fen[cur_idx];
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
                    castle |= k;
                    break;
                case 'q':
                    castle |= q;
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
            std::string tmp = fen.substr(cur_idx, 2);
            half_move_clock = (tmp[0] - '0') * 10 + tmp[1] - '0';
            cur_idx += 3;
        }

        if (cur_idx + 1 < fen.size()) {
            std::string tmp = fen.substr(cur_idx, 2);
            full_move_counter = ((tmp[0] - '0') * 10) + tmp[1] - '0';
        } else {
            full_move_counter = fen[cur_idx] - '0';
        }

        update_occupancies();
    }
};

int main() {
    Board chess;
    std::string fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    chess.parseFEN(fen);

    chess.print_board();

    return 0;
}
