#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "uci.h"
#include "bench.h"
#include "movegen.h"
#include "search.h"
#include "tt.h"

std::string move_to_uci(int move) {
    std::string uci = Board::int_to_sq(get_move_source(move))
                    + Board::int_to_sq(get_move_target(move));

    int promo = get_move_promoted(move);
    if (promo) {
        char c = 'q';
        if (promo == R || promo == r) c = 'r';
        else if (promo == B || promo == b) c = 'b';
        else if (promo == N || promo == n) c = 'n';
        uci += c;
    }

    return uci;
}

int parse_uci_move(Board* board, const std::string& move_string) {
    Moves move_list[1];
    generate_moves(board, move_list);

    int source = (move_string[0] - 'a') + ((move_string[1] - '1') * 8);
    int target = (move_string[2] - 'a') + ((move_string[3] - '1') * 8);

    for (int i = 0; i < move_list->count; i++) {
        int move = move_list->moves[i];

        if (get_move_source(move) == source && get_move_target(move) == target) {
            int promoted = get_move_promoted(move);

            if (promoted) {
                if (move_string.length() > 4) {
                    char promo_char = move_string[4];
                    if ((promoted == Q || promoted == q) && promo_char == 'q') return move;
                    if ((promoted == R || promoted == r) && promo_char == 'r') return move;
                    if ((promoted == B || promoted == b) && promo_char == 'b') return move;
                    if ((promoted == N || promoted == n) && promo_char == 'n') return move;
                    continue;
                }
            }
            return move;
        }
    }
    return 0;
}

void parse_position(Board* board, std::string command) {
    std::stringstream ss(command);
    std::string token;
    ss >> token; // "position"

    ss >> token; // "startpos" or "fen"

    if (token == "startpos") {
        board->parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        ss >> token; // possibly "moves"
    }
    else if (token == "fen") {
        // Collect the FEN fields until "moves" or end of line. Don't assume a
        // fixed field count: some GUIs omit the halfmove/fullmove counters.
        std::string fen_full;
        while (ss >> token && token != "moves") {
            fen_full += token + " ";
        }
        board->parseFEN(fen_full);
        // token is now "moves" (handled below) or unchanged at end of stream.
    }

    game_history_reset();
    game_history_push(board->hash_key);

    if (token == "moves") {
        std::string move_str;
        while (ss >> move_str) {
            int move = parse_uci_move(board, move_str);
            if (move != 0) {
                make_move(board, move, 0);
                game_history_push(board->hash_key);
            }
        }
    }
}

void parse_go(Board* board, std::string command) {
    SearchLimits limits;
    int perft_depth = 0;

    std::stringstream ss(command);
    std::string token;
    ss >> token;

    while (ss >> token) {
        if (token == "depth") {
            ss >> limits.depth;
        } else if (token == "movetime") {
            ss >> limits.movetime;
        } else if (token == "wtime") {
            ss >> limits.wtime;
        } else if (token == "btime") {
            ss >> limits.btime;
        } else if (token == "winc") {
            ss >> limits.winc;
        } else if (token == "binc") {
            ss >> limits.binc;
        } else if (token == "movestogo") {
            ss >> limits.movestogo;
        } else if (token == "infinite") {
            limits.infinite = true;
        } else if (token == "perft") {
            ss >> perft_depth;
        }
    }

    if (perft_depth > 0) {
        long long total = uci_perft(board, perft_depth);
        std::cout << total << std::endl;
        return;
    }

    search_position(board, limits);
}

void uci_loop() {
    Board board;
    std::string line, token;

    std::cout.setf(std::ios::unitbuf);

    while (std::getline(std::cin, line)) {
        std::stringstream ss(line);
        ss >> token;

        if (token == "uci") {
            std::cout << "id name " << ENGINE_NAME << std::endl;
            std::cout << "id author " << ENGINE_AUTHOR << std::endl;
            std::cout << "uciok" << std::endl;
        }
        else if (token == "isready") {
            std::cout << "readyok" << std::endl;
        }
        else if (token == "position") {
            parse_position(&board, line);
        }
        else if (token == "ucinewgame") {
            clear_tt();
            parse_position(&board, "position startpos");
        }
        else if (token == "go") {
            parse_go(&board, line);
        }
        else if (token == "stop") {
            request_stop();
        }
        else if (token == "bench") {
            int depth = BENCH_DEFAULT_DEPTH;
            ss >> depth;
            run_bench(depth, false);
        }
        else if (token == "quit") {
            break;
        }
    }
}
