#include <chrono>
#include <iostream>

#include "bench.h"
#include "board.h"
#include "search.h"
#include "tt.h"

// Fixed positions (Stockfish-style bench set) for repeatable node-count regression.
static const char* BENCH_FENS[] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p2K1/8/5Q2/8/BB3PPN/7k w - - 0 9",
    "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2",
    "rnbqkbnr/pp2pppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2",
    "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 0 2",
    "rnbqkbnr/pppp2pp/5p2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 3",
    "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq e3 0 2",
    "rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2",
    "rnbqkbnr/pp2pppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2",
    "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 0 2",
    "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 0 2",
    "rnbqkbnr/pppp2pp/5p2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 3",
    "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2",
    "6k1/5ppp/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
    "3r2k1/8/8/8/8/8/5PPP/5RK1 w - - 0 1",
};

static constexpr int BENCH_POSITION_COUNT =
    static_cast<int>(sizeof(BENCH_FENS) / sizeof(BENCH_FENS[0]));

BenchResult run_bench(int depth, bool quiet) {
    Board board;
    BenchResult result;
    result.depth = depth;
    result.positions = BENCH_POSITION_COUNT;

    clear_tt();

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < BENCH_POSITION_COUNT; i++) {
        board.parseFEN(BENCH_FENS[i]);
        game_history_reset();
        game_history_push(board.hash_key);

        SearchLimits limits;
        limits.depth = depth;
        limits.quiet = true;

        search_position(&board, limits);
        result.total_nodes += get_search_nodes();

        if (!quiet) {
            std::cout << "Position: " << (i + 1) << "/" << BENCH_POSITION_COUNT << std::endl;
        }
    }

    auto end = std::chrono::steady_clock::now();
    result.time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    if (result.time_ms == 0) {
        result.time_ms = 1;
    }
    result.nps = result.total_nodes * 1000 / result.time_ms;

    std::cout << "==========================="
              << "\nTotal time (ms) : " << result.time_ms
              << "\nNodes searched  : " << result.total_nodes
              << "\nNodes/second    : " << result.nps
              << std::endl;

    return result;
}
