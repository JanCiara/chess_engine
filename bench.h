#pragma once

inline constexpr int BENCH_DEFAULT_DEPTH = 8;

struct BenchResult {
    long long total_nodes = 0;
    long long time_ms = 0;
    long long nps = 0;
    int depth = 0;
    int positions = 0;
};

BenchResult run_bench(int depth, bool quiet);
