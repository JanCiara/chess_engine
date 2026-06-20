# Chess Engine

[![CI](https://img.shields.io/github/actions/workflow/status/JanCiara/chess_engine/ci.yml?branch=main&label=CI)](https://github.com/JanCiara/chess_engine/actions/workflows/ci.yml)

**C++20 UCI chess engine — ~900k NPS search benchmark (Release, depth 8)**

Bitboard move generation, Negamax search with alpha-beta pruning, and a Zobrist-backed transposition table. Built for speed and correctness, with regression tests on every push.

**Author:** janek

---

## Performance

| Benchmark | Result | Notes |
|-----------|--------|-------|
| **Search bench** | **~900k NPS** | 48-position Stockfish-style suite, depth 8, ~33M nodes |
| **Perft** | **~19M NPS** | Starting position, depth 5 (~4.9M nodes) |
| **WAC tactics** | **76%** (23/30) | Win At Chess suite at depth 10 |

Numbers vary by CPU and compiler; run the commands below on your machine to reproduce.

```
bench 8
```

Example output:

```
===========================
Total time (ms) : 36557
Nodes searched  : 33096264
Nodes/second    : 905333
```

Perft speed (move generation only):

```bash
./build/perft_test   # prints benchmark depth=5 ... nps=...
```

---

## Architecture

**Bitboards** — Board state uses 64-bit bitboards per piece type. Sliders use magic bitboards (precomputed attack tables); leapers and pawns use lookup tables. Moves are generated incrementally with make/unmake and a compact undo stack.

**Negamax** — Iterative deepening with alpha-beta pruning, aspiration windows, null-move pruning, late move reductions (LMR), check extensions, and quiescence search (captures and promotions). Move ordering: hash move from TT, PV move from the previous iteration, MVV-LVA for captures.

**Transposition table (TT)** — 16-byte entries (4 per cache line) keyed by Zobrist hash; stores depth, score, bound flag, and best move for cutoffs and move ordering.

Additional features: piece-square evaluation, 50-move / threefold / insufficient-material draw detection, UCI time management (`wtime`, `btime`, `movetime`, increments).

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
| `bench_test` | Search bench node count at depth 8 (regression) |
| `wac_test` | WAC tactical suite at depth 10 |
| `eval_test` | Evaluation score regression |

---

## UCI

Connect to any UCI GUI (Arena, Cute Chess, Lichess Bot, etc.):

```bash
./build/chess_engine
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
| `board.cpp` | Board state, FEN parsing |
| `movegen.cpp` | Bitboard movegen, magic bitboards, perft |
| `evaluate.cpp` | Piece-square tables |
| `search.cpp` | Negamax, iterative deepening |
| `tt.cpp` | Zobrist hashing, transposition table |
| `draw.cpp` | Draw detection |
| `bench.cpp` | Fixed-position search benchmark |
| `uci.cpp` | UCI protocol |

Automated Elo/SPRT testing against reference engines: `python run_elo_tests.py` (requires [cutechess-cli](https://github.com/cutechess/cutechess)).
