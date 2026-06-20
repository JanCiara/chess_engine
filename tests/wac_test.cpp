#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "board.h"
#include "epd.h"
#include "movegen.h"
#include "search.h"
#include "tt.h"
#include "uci.h"

#ifndef WAC_EPD_PATH
#define WAC_EPD_PATH "tests/wac.epd"
#endif

static constexpr int WAC_SEARCH_DEPTH = 10;

static bool move_is_solution(Board* board, int move, const EpdPosition& position) {
    if (move == 0) {
        return false;
    }

    std::string uci = move_to_uci(move);
    for (const auto& expected : position.best_moves) {
        if (uci == expected) {
            return true;
        }
    }
    return false;
}

int main() {
    init_zobrist();
    init_all_attacks();

    std::ifstream input(WAC_EPD_PATH);
    if (!input) {
        std::cerr << "failed to open EPD file: " << WAC_EPD_PATH << std::endl;
        return EXIT_FAILURE;
    }

    Board board;
    int total = 0;
    int solved = 0;
    std::string line;

    while (std::getline(input, line)) {
        EpdPosition position;
        if (!parse_epd_line(line, &position)) {
            continue;
        }

        total++;
        board.parseFEN(position.fen);
        game_history_reset();
        game_history_push(board.hash_key);

        SearchLimits limits;
        limits.depth = WAC_SEARCH_DEPTH;
        limits.quiet = true;
        search_position(&board, limits);

        bool pass = move_is_solution(&board, best_move, position);
        if (pass) {
            solved++;
        }

        std::cout << (pass ? "[PASS] " : "[FAIL] ")
                  << position.id
                  << " best=" << (best_move ? move_to_uci(best_move) : "(none)")
                  << " expected=";
        for (size_t i = 0; i < position.best_moves.size(); i++) {
            if (i > 0) {
                std::cout << ' ';
            }
            std::cout << position.best_moves[i];
        }
        std::cout << std::endl;
    }

    if (total == 0) {
        std::cerr << "no EPD positions loaded from " << WAC_EPD_PATH << std::endl;
        return EXIT_FAILURE;
    }

    int percent = solved * 100 / total;
    std::cout << "wac depth=" << WAC_SEARCH_DEPTH
              << " solved=" << solved << "/" << total
              << " (" << percent << "%)" << std::endl;

    return EXIT_SUCCESS;
}
