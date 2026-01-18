#include <iostream>
#include "board.h"
#include "movegen.h"
#include "evaluate.h" // <--- Include

int main() {
    init_all_attacks();
    Board chess;

    chess.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    std::cout << "Start Position Evaluation: " << evaluate(&chess) << "\n";

    chess.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNB1KBNR w KQkq - 0 1");
    std::cout << "No Queen Evaluation: " << evaluate(&chess) << "\n";

    return 0;
}