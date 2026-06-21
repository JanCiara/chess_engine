#include <cstdlib>
#include <iostream>

#include "bench.h"
#include "tt.h"
#include "movegen.h"

// Total nodes at depth 8 over the fixed bench set (search regression baseline).
static constexpr long long EXPECTED_BENCH_NODES = 21885647LL;

int main() {
    init_zobrist();

    BenchResult result = run_bench(BENCH_DEFAULT_DEPTH, true);

    std::cout << "bench depth=" << result.depth
              << " nodes=" << result.total_nodes
              << " time_ms=" << result.time_ms
              << " nps=" << result.nps << std::endl;

    if (EXPECTED_BENCH_NODES > 0 && result.total_nodes != EXPECTED_BENCH_NODES) {
        std::cout << "[FAIL] bench nodes=" << result.total_nodes
                  << " expected=" << EXPECTED_BENCH_NODES << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "[PASS] bench completed" << std::endl;
    return EXIT_SUCCESS;
}
