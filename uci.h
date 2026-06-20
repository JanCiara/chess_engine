#pragma once
#include "board.h"
#include "search.h"

inline constexpr const char* ENGINE_NAME = "Chess Engine";
inline constexpr const char* ENGINE_AUTHOR = "janek";

int parse_uci_move(Board* board, const std::string& move_string);
std::string move_to_uci(int move);

void uci_loop();