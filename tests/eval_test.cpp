#include <cstdlib>
#include <iostream>
#include <string>

#include "board.h"
#include "evaluate.h"
#include "movegen.h"
#include "tt.h"

struct EvalCase {
    const char* name;
    const char* fen_better;
    const char* fen_worse;
    int min_delta;
};

static const EvalCase EVAL_CASES[] = {
    {
        "connected pawns beat isolated",
        "8/8/8/2PP4/8/8/8/4K2k w - - 0 1",
        "8/8/8/P2P4/8/8/8/4K2k w - - 0 1",
        10,
    },
    {
        "single pawn beats doubled pawns",
        "8/8/8/3PP3/8/8/8/4K2k w - - 0 1",
        "8/8/8/4PP3/8/8/8/4K2k w - - 0 1",
        10,
    },
    {
        "passed pawn beats blocked pawn",
        "8/4k3/3P4/8/8/8/8/4K3 w - - 0 1",
        "8/3kp3/3P4/8/8/8/8/4K3 w - - 0 1",
        20,
    },
    {
        "central knight beats corner knight",
        "8/8/8/8/4N3/8/8/4K2k w - - 0 1",
        "N7/8/8/8/8/8/8/4K2k w - - 0 1",
        15,
    },
    {
        "castled king beats exposed king",
        "6kq1/5ppp/8/8/8/8/5PPP/5RK1 w - - 0 1",
        "6kq1/5ppp/8/8/8/8/5PPP/4K3 w - - 0 1",
        10,
    },
};

static int eval_fen(const char* fen) {
    Board board;
    board.parseFEN(fen);
    return evaluate(&board);
}

static bool run_case(const EvalCase& test) {
    int better = eval_fen(test.fen_better);
    int worse = eval_fen(test.fen_worse);
    int delta = better - worse;
    bool pass = delta >= test.min_delta;

    std::cout << (pass ? "[PASS] " : "[FAIL] ")
              << test.name << ": better=" << better
              << " worse=" << worse
              << " delta=" << delta
              << " min=" << test.min_delta << std::endl;

    return pass;
}

int main() {
    init_zobrist();
    init_all_attacks();

    int failed = 0;
    for (const auto& test : EVAL_CASES) {
        if (!run_case(test)) {
            failed++;
        }
    }

    if (failed > 0) {
        std::cout << failed << " eval case(s) failed" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "all eval cases passed" << std::endl;
    return EXIT_SUCCESS;
}
