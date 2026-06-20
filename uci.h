#pragma once
#include "board.h"

inline constexpr const char* ENGINE_NAME = "Chess Engine";
inline constexpr const char* ENGINE_AUTHOR = "janek";

int parse_uci_move(Board* board, const std::string& move_string);

inline void move_to_uci(int move, char uci[6]) {
    Board::sq_to_chars(get_move_source(move), uci);
    Board::sq_to_chars(get_move_target(move), uci + 2);

    const int promo = get_move_promoted(move);
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
}

void uci_loop();
