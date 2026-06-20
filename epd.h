#pragma once

#include <string>
#include <vector>

struct EpdPosition {
    std::string fen;
    std::vector<std::string> best_moves;
    std::string id;
};

bool parse_epd_line(const std::string& line, EpdPosition* out);
