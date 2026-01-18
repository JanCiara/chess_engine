#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "uci.h"
#include "movegen.h"

// Helper: Convert string move (e.g., "e2e4") to internal int move
int parse_move(Board* board, std::string move_string) {
    Moves move_list[1];
    generate_moves(board, move_list);

    // FIX: Parse coordinates for Little-Endian Bitboards (A1 = 0)
    // File: 'a' -> 0, 'h' -> 7
    // Rank: '1' -> 0, '8' -> 7 (multiply by 8 for row)

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

// Parse "position" command
// Examples:
// "position startpos"
// "position startpos moves e2e4 e7e5"
// "position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 moves e2e4"
void parse_position(Board* board, std::string command) {
    std::stringstream ss(command);
    std::string token;
    ss >> token; // "position"

    ss >> token; // "startpos" or "fen"
    
    if (token == "startpos") {
        board->parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        ss >> token;
    } 
    else if (token == "fen") {
        std::string fen_part, fen_full = "";
        for (int i = 0; i < 6; ++i) {
             ss >> fen_part;
             fen_full += fen_part + " ";
        }
        board->parseFEN(fen_full.c_str());
        ss >> token;
    }

    if (token == "moves") {
        std::string move_str;
        while (ss >> move_str) {
            int move = parse_move(board, move_str);
            if (move != 0) {
                make_move(board, move, 0);
            }
        }
    }
}

void parse_go(Board* board, std::string command) {
    int depth = 4;

    std::stringstream ss(command);
    std::string token;
    ss >> token;

    while (ss >> token) {
        if (token == "depth") {
            ss >> depth;
        }
    }

    search_position(board, depth);
}

void uci_loop() {
    Board board;
    std::string line, token;

    std::cout.setf(std::ios::unitbuf);

    while (std::getline(std::cin, line)) {
        std::stringstream ss(line);
        ss >> token;

        if (token == "uci") {
            std::cout << "id name MyEngine1" << std::endl;
            std::cout << "id author You" << std::endl;
            std::cout << "uciok" << std::endl;
        }
        else if (token == "isready") {
            std::cout << "readyok" << std::endl;
        }
        else if (token == "position") {
            parse_position(&board, line);
        }
        else if (token == "ucinewgame") {
            parse_position(&board, "position startpos");
        }
        else if (token == "go") {
            parse_go(&board, line);
        }
        else if (token == "quit") {
            break;
        }
    }
}