#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include "uci.h"
#include "bench.h"
#include "movegen.h"
#include "search.h"
#include "tt.h"

static std::thread search_thread;

static void stop_and_join_search(Search& search) {
    search.request_stop();
    if (search_thread.joinable()) {
        search_thread.join();
    }
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

void parse_position(Board* board, Search* search, std::string command) {
    std::stringstream ss(command);
    std::string token;
    ss >> token; // "position"

    ss >> token; // "startpos" or "fen"

    if (token == "startpos") {
        board->parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        ss >> token; // possibly "moves"
    }
    else if (token == "fen") {
        std::string fen_full;
        while (ss >> token && token != "moves") {
            fen_full += token + " ";
        }
        board->parseFEN(fen_full);
    }

    search->game_history_reset();
    search->game_history_push(board->hash_key());

    if (token == "moves") {
        std::string move_str;
        while (ss >> move_str) {
            int move = parse_uci_move(board, move_str);
            if (move != 0) {
                Undo undo;
                make_move(board, move, &undo, 0);
                search->game_history_push(board->hash_key());
            }
        }
    }
}

void parse_go(Board* board, Search* search, std::string command) {
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
        stop_and_join_search(*search);
        long long total = uci_perft(board, perft_depth);
        std::cout << total << std::endl;
        return;
    }

    stop_and_join_search(*search);
    search->request_stop();

    search_thread = std::thread([board, search, limits]() {
        search->search_position(board, limits);
    });
}

void uci_loop() {
    Board board;
    Search search;
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
            stop_and_join_search(search);
            search.request_stop();
            std::cout << "readyok" << std::endl;
        }
        else if (token == "position") {
            stop_and_join_search(search);
            parse_position(&board, &search, line);
        }
        else if (token == "ucinewgame") {
            stop_and_join_search(search);
            clear_tt();
            parse_position(&board, &search, "position startpos");
        }
        else if (token == "go") {
            parse_go(&board, &search, line);
        }
        else if (token == "stop") {
            search.request_stop();
        }
        else if (token == "bench") {
            stop_and_join_search(search);
            int depth = BENCH_DEFAULT_DEPTH;
            ss >> depth;
            run_bench(depth, false);
        }
        else if (token == "quit") {
            stop_and_join_search(search);
            break;
        }
    }
}
