#include <cctype>
#include <sstream>

#include "epd.h"

static std::string trim(const std::string& text) {
    size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        start++;
    }

    size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        end--;
    }

    return text.substr(start, end - start);
}

static void parse_bm_moves(const std::string& text, std::vector<std::string>* moves) {
    std::stringstream ss(text);
    std::string move;
    while (ss >> move) {
        moves->push_back(move);
    }
}

static void parse_id_value(const std::string& text, std::string* id) {
    std::string value = trim(text);
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        *id = value.substr(1, value.size() - 2);
        return;
    }
    *id = value;
}

bool parse_epd_line(const std::string& line, EpdPosition* out) {
    if (!out) {
        return false;
    }

    out->fen.clear();
    out->best_moves.clear();
    out->id.clear();

    std::string trimmed = trim(line);
    if (trimmed.empty() || trimmed[0] == '#') {
        return false;
    }

    size_t pos = 0;
    std::string fen;
    int fen_fields = 0;

    while (pos < trimmed.size() && fen_fields < 4) {
        while (pos < trimmed.size() && std::isspace(static_cast<unsigned char>(trimmed[pos]))) {
            pos++;
        }
        if (pos >= trimmed.size()) {
            break;
        }

        size_t start = pos;
        while (pos < trimmed.size() && !std::isspace(static_cast<unsigned char>(trimmed[pos]))) {
            pos++;
        }

        if (!fen.empty()) {
            fen += ' ';
        }
        fen += trimmed.substr(start, pos - start);
        fen_fields++;
    }

    if (fen_fields < 4) {
        return false;
    }

    while (pos < trimmed.size() && std::isspace(static_cast<unsigned char>(trimmed[pos]))) {
        pos++;
    }

    if (pos < trimmed.size()) {
        size_t start = pos;
        while (pos < trimmed.size() && !std::isspace(static_cast<unsigned char>(trimmed[pos]))) {
            pos++;
        }
        fen += ' ';
        fen += trimmed.substr(start, pos - start);

        while (pos < trimmed.size() && std::isspace(static_cast<unsigned char>(trimmed[pos]))) {
            pos++;
        }
        if (pos < trimmed.size()) {
            start = pos;
            while (pos < trimmed.size() && !std::isspace(static_cast<unsigned char>(trimmed[pos]))) {
                pos++;
            }
            fen += ' ';
            fen += trimmed.substr(start, pos - start);
        }
    }

    out->fen = fen;

    while (pos < trimmed.size()) {
        while (pos < trimmed.size() && std::isspace(static_cast<unsigned char>(trimmed[pos]))) {
            pos++;
        }
        if (pos >= trimmed.size()) {
            break;
        }

        size_t op_start = pos;
        while (pos < trimmed.size() && trimmed[pos] != ';') {
            pos++;
        }

        std::string operation = trim(trimmed.substr(op_start, pos - op_start));
        if (pos < trimmed.size()) {
            pos++;
        }

        if (operation.rfind("bm ", 0) == 0) {
            parse_bm_moves(operation.substr(3), &out->best_moves);
        } else if (operation.rfind("id ", 0) == 0) {
            parse_id_value(operation.substr(3), &out->id);
        }
    }

    return !out->best_moves.empty();
}
