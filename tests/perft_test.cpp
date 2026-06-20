#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

#include "board.h"
#include "movegen.h"
#include "tt.h"

struct PerftCase {
    const char* name;
    const char* fen;
    int depth;
    long long expected;
};

static const PerftCase PERFT_CASES[] = {
    {"startpos depth 1", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1, 20},
    {"startpos depth 2", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 2, 400},
    {"startpos depth 3", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 3, 8902},
    {"startpos depth 4", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4, 197281},
    {"kiwipete depth 3", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3, 97862},
    {"kiwipete depth 4", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603},
    {"castling rights depth 1", "48k4k/8/8/8/8/8/4K3P/7k w - - 0 1", 1, 9},
    {"castling depth 1", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 1, 48},
};

static long long run_perft(Board* board, int depth) {
    return perft_driver(depth, board);
}

static bool run_case(const PerftCase& test) {
    Board board;
    board.parseFEN(test.fen);

    long long nodes = run_perft(&board, test.depth);
    bool pass = nodes == test.expected;

    std::cout << (pass ? "[PASS] " : "[FAIL] ")
              << test.name << ": nodes=" << nodes
              << " expected=" << test.expected << std::endl;

    return pass;
}

static void run_benchmark() {
    Board board;
    board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    constexpr int depth = 5;
    auto start = std::chrono::steady_clock::now();
    long long nodes = run_perft(&board, depth);
    auto end = std::chrono::steady_clock::now();

    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    if (ms == 0) {
        ms = 1;
    }

    long long nps = nodes * 1000 / ms;

    std::cout << "benchmark depth=" << depth
              << " nodes=" << nodes
              << " time_ms=" << ms
              << " nps=" << nps << std::endl;
}

int main() {
    init_zobrist();

    int failed = 0;
    for (const auto& test : PERFT_CASES) {
        if (!run_case(test)) {
            failed++;
        }
    }

    run_benchmark();

    if (failed > 0) {
        std::cout << failed << " perft case(s) failed" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "all perft cases passed" << std::endl;
    return EXIT_SUCCESS;
}
