# Chess Engine

[![CI](https://img.shields.io/github/actions/workflow/status/JanCiara/chess_engine/ci.yml?branch=main&label=CI)](https://github.com/JanCiara/chess_engine/actions/workflows/ci.yml)

A bitboard-based chess engine with UCI protocol support.

**Author:** janek

## Features

- Bitboard move generation (magic bitboards for sliders)
- Negamax search with alpha-beta pruning
- Iterative deepening and time management (`wtime` / `btime` / `movetime`)
- Quiescence search (captures and promotions)
- MVV-LVA move ordering and PV move from previous iteration
- Transposition table (Zobrist hashing)
- Draw detection: 50-move rule, threefold repetition, insufficient material

## Requirements

- CMake 3.20 or newer
- C++20 compiler (GCC 10+, Clang 12+, MSVC 2019 16.11+)

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

On Windows with Visual Studio:

```powershell
cmake -B build
cmake --build build --config Release
```

The executable is written to `build/chess_engine` (Unix) or `build/Release/chess_engine.exe` (MSVC multi-config).

Release builds use `-O2 -march=native` (GCC/Clang) or `/O2` (MSVC).

### Run tests

```bash
cmake --build build
ctest --test-dir build --output-on-failure
# or directly:
./build/perft_test
./build/bench_test
./build/wac_test
```

| Test | What it checks |
|------|----------------|
| `perft_test` | Move generation node counts + perft NPS benchmark |
| `bench_test` | Fixed-position search bench: repeatable total nodes at depth 8 |
| `wac_test` | WAC EPD tactical suite: % of positions with correct best move (depth 10) |

Results are published in the [CI workflow summary](https://github.com/JanCiara/chess_engine/actions/workflows/ci.yml) on every push.

## UCI usage

Run the engine and connect it to a UCI-compatible GUI (Arena, Cute Chess, Lichess Bot, etc.):

```bash
./build/chess_engine
```

Example commands:

```
uci
isready
position startpos
go depth 8
position startpos moves e2e4 e7e5
go wtime 60000 btime 60000 winc 1000 binc 1000
go perft 5
bench
bench 8
quit
```

### Supported `go` options

| Option | Description |
|--------|-------------|
| `depth N` | Search to depth N |
| `movetime N` | Search for N milliseconds |
| `wtime` / `btime` | Remaining clock (ms) |
| `winc` / `binc` | Increment per move (ms) |
| `movestogo` | Moves until time control |
| `infinite` | Search until `stop` |
| `perft N` | Perft division to depth N |

### `bench`

Stockfish-style search benchmark over a fixed set of 48 positions. Runs a quiet depth-limited search on each position and prints total nodes and NPS — useful for regression testing and comparing builds:

```
bench      # default depth 8
bench 10   # custom depth
```

Example output:

```
===========================
Total time (ms) : 36557
Nodes searched  : 33096264
Nodes/second    : 905333
```

## Project layout

| File | Role |
|------|------|
| `board.cpp` | Board state, FEN parsing |
| `movegen.cpp` | Move generation, `make_move`, perft |
| `evaluate.cpp` | Piece-square tables evaluation |
| `search.cpp` | Search, iterative deepening |
| `tt.cpp` | Zobrist keys and transposition table |
| `draw.cpp` | Draw detection helpers |
| `bench.cpp` | Fixed-position search benchmark |
| `epd.cpp` | EPD line parser (`bm`, `id`) |
| `uci.cpp` | UCI protocol loop |
| `tests/perft_test.cpp` | Perft regression suite and NPS benchmark |
| `tests/bench_test.cpp` | Search bench regression (node count) |
| `tests/wac_test.cpp` | WAC tactical EPD suite |
| `tests/wac.epd` | 30 WAC positions (UCI `bm` moves) |

## Perft sanity check

From the starting position, depth 3 should report **8902** nodes:

```
position startpos
go perft 3
```

## WAC tactical suite

The engine loads `tests/wac.epd` (30 positions from the classic Win At Chess set) and reports the solve rate at depth 10, e.g.:

```
wac depth=10 solved=23/30 (76%)
```

Run via `ctest` or `./build/wac_test`.
