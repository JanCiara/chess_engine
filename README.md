# Chess Engine

[![CI](https://img.shields.io/github/actions/workflow/status/JanCiara/chess_engine/ci.yml?branch=main&label=CI)](https://github.com/JanCiara/chess_engine/actions/workflows/ci.yml)

**C++20 UCI chess engine — bitboard movegen, Negamax search, Zobrist TT**

Bitboard move generation, Negamax search with alpha-beta pruning, and a Zobrist-backed transposition table. Built for speed and correctness, with regression tests on every push.

**Author:** janek

---

## Performance

| Benchmark | Result | Notes |
|-----------|--------|-------|
| **Search bench** | **~22M nodes @ depth 8** | 48-position Stockfish-style suite; NPS depends on CPU (see below) |
| **Perft** | **~4.9M nodes @ depth 5** | Starting position; movegen-only speed varies by CPU |
| **WAC tactics** | **76%** (23/30) | Win At Chess suite at depth 10 |
| **Elo vs Stockfish** | **~2130** | 100 valid games @ 10+0.1 vs UCI_Elo=1800 |

NPS and wall-clock times vary by CPU, compiler, and `-march=native` / LTO. The **node counts** above are fixed regression baselines — if search logic changes, update `tests/bench_test.cpp` and re-run the suite.

```
bench 8
```

Example output (Release, typical modern desktop):

```
===========================
Total time (ms) : 5700
Nodes searched  : 21885647
Nodes/second    : 3837567
```

Perft speed (move generation only):

```bash
./build/perft_test   # prints benchmark depth=5 ... nps=...
# Windows: build\Release\perft_test.exe
```

Example: `benchmark depth=5 nodes=4865609 time_ms=114 nps=42680780` (Release, MSVC, recent CPU).

### Elo testing (vs Stockfish)

100-game blitz match (`10+0.1`, 2-move UHO book, concurrency 1):

```bash
python run_elo_tests.py --games 100 --concurrency 1 --no-sprt
python analyze_elo.py    # -> testing/results_valid.pgn
```

| | |
|---|---|
| **Valid games** | **100 / 100** |
| **Score** | **87–13–0** (87%) |
| **vs Stockfish (UCI_Elo=1800)** | **+330 ± 233 Elo** |
| **Estimated rating** | **~2130** |

Reference opponent is [Stockfish](https://github.com/official-stockfish/Stockfish) with `UCI_LimitStrength` / `UCI_Elo=1800` (auto-downloaded). Use `--concurrency 1` to avoid UCI stalls under load.

---

## Architecture

**Bitboards** — Board state uses 64-bit bitboards per piece type plus a square mailbox for O(1) `piece_at`. Sliders use magic bitboards (precomputed attack tables); leapers and pawns use lookup tables. Moves are generated incrementally with make/unmake and a compact undo stack.

**Zobrist hashing** — Position hash is updated incrementally on make/null-move (full recompute only when loading FEN or constructing the start position). Keys drive the transposition table and repetition detection in search.

**Negamax** — Iterative deepening with alpha-beta pruning, aspiration windows, null-move pruning, late move reductions (LMR), check extensions, and quiescence search (captures and promotions). Move ordering: PV move, TT move, SEE for captures/promotions, killer moves for quiet cutoffs.

**Transposition table (TT)** — 16-byte entries (4 per cache line) keyed by Zobrist hash; stores depth, score, bound flag, and best move for cutoffs and move ordering.

**Evaluation** — Phased MG/EG piece-square tables with interpolation, pawn structure, mobility, and king safety.

**Draw rules** — 50-move rule, insufficient material, repetition within the current search (plus game history from UCI `position`).

**UCI** — Time management (`wtime`, `btime`, `movetime`, increments), search on a worker thread with cooperative `stop`.

---

## Build

**Requirements:** CMake 3.20+, C++20 (GCC 10+, Clang 12+, MSVC 2019 16.11+)

### Linux / macOS

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Binary: `build/chess_engine`

Release builds use `-O2 -march=native` (GCC/Clang). Link-time optimization (LTO) is enabled by default.

### Windows (Visual Studio)

```powershell
cmake -B build
cmake --build build --config Release
```

Binary: `build/Release/chess_engine.exe`

### Tests

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

On Windows, add `-C Release` to `ctest`. CI publishes bench and perft numbers in the [workflow summary](https://github.com/JanCiara/chess_engine/actions/workflows/ci.yml) on every push.

| Target | What it checks |
|--------|----------------|
| `unit_test` | Bitboards, move encoding, FEN/EPD, Zobrist, draw rules, TT |
| `perft_test` | Perft node counts + perft NPS |
| `bench_test` | Search bench node count at depth 8 (`21885647` nodes baseline) |
| `wac_test` | WAC tactical suite at depth 10 |
| `eval_test` | Evaluation score regression |

---

## UCI

Connect to any UCI GUI (Arena, Cute Chess, Lichess Bot, etc.):

```bash
./build/chess_engine
# Windows: build\Release\chess_engine.exe
```

```
uci
isready
position startpos
go depth 8
bench 8
go perft 5
quit
```

| `go` option | Description |
|-------------|-------------|
| `depth N` | Search to depth N |
| `movetime N` | Search for N ms |
| `wtime` / `btime` | Remaining clock (ms) |
| `winc` / `binc` | Increment per move (ms) |
| `movestogo` | Moves until time control |
| `infinite` | Search until `stop` |
| `perft N` | Perft division to depth N |

---

## Project layout

| File | Role |
|------|------|
| `board.cpp` | Board state, mailbox, FEN parsing |
| `movegen.cpp` | Bitboard movegen, magic bitboards, make/unmake, perft |
| `evaluate.cpp` | Phased evaluation (PST, structure, mobility, king safety) |
| `search.cpp` | Negamax, iterative deepening, SEE, killers |
| `tt.cpp` | Zobrist keys, incremental hash helpers, transposition table |
| `draw.cpp` | Draw detection |
| `bench.cpp` | Fixed-position search benchmark |
| `uci.cpp` | UCI protocol |

Automated Elo/SPRT testing: `python run_elo_tests.py` · analysis: `python analyze_elo.py` (requires [cutechess-cli](https://github.com/cutechess/cutechess)).

---

## Refinement (no new features)

Items worth polishing in the existing codebase — tuning and quality, not scope creep:

| Area | What to improve |
|------|-----------------|
| **Transposition table** | Depth-preferred replacement instead of always overwriting; optional age/generation bucket |
| **Evaluation** | Hand-tune MG/EG weights; reduce duplicated white/black loops in `evaluate.cpp`; target higher WAC pass rate at depth 10 |
| **Search tuning** | Aspiration window size, LMR thresholds, null-move depth/reduction — tune against fixed suites without changing algorithms |
| **Time management** | `allocate_time_ms` fractions and safety margins; validate under blitz increments with Elo script |
| **Repetition** | Align search-side repetition with full game history from UCI (today partially covered) |
| **FEN / UCI input** | Replace silent `std::cout` on bad FEN with clear failure; validate token count and castling/en passant fields |
| **Code consistency** | Unify comment language (PL/EN mix today); small cleanups in `search_root` (duplicate best-move assignment) |
| **Bench suite** | Positions 16–48 repeat the same FEN — trim duplicates or document why they stress TT/cache |
| **Tests** | Add make/unmake hash regression (`hash_key == compute_hash` after random legal moves); optional perft in `unit_test` CI path |
| **Documentation** | Keep `EXPECTED_BENCH_NODES` in README and `bench_test.cpp` in sync after any search change |
