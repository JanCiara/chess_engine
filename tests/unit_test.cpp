#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <gtest/gtest.h>

#ifdef WHITE
#undef WHITE
#endif
#ifdef BLACK
#undef BLACK
#endif

#include "board.h"
#include "defs.h"
#include "draw.h"
#include "epd.h"
#include "tt.h"

class EngineFixture : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        init_zobrist();
    }

    void SetUp() override {
        clear_tt();
    }
};

// --- Bitboard helpers (defs.h) — pure functions, no engine state ---

TEST(BitboardHelpers, SetGetPopBit) {
    U64 bb = 0;
    set_bit(bb, e4);
    EXPECT_TRUE(get_bit(bb, e4));
    EXPECT_FALSE(get_bit(bb, d4));
    pop_bit(bb, e4);
    EXPECT_EQ(bb, 0ULL);
}

TEST(BitboardHelpers, CountBitsAndLsb) {
    U64 bb = (1ULL << a1) | (1ULL << h8) | (1ULL << d4);
    EXPECT_EQ(count_bits(bb), 3);
    EXPECT_EQ(get_LSB(bb), a1);
    EXPECT_EQ(pop_LSB(bb), a1);
    EXPECT_EQ(count_bits(bb), 2);
}

TEST(BitboardHelpers, RowCol) {
    EXPECT_EQ(ROW(e4), 3);
    EXPECT_EQ(COL(e4), 4);
    EXPECT_EQ(ROW(a8), 7);
    EXPECT_EQ(COL(h1), 7);
}

// --- Move encoding (defs.h) ---

TEST(MoveEncoding, RoundTripAndFlags) {
    const int move = encode_move(e2, e4, P, 0, 0, 1, 0, 0);
    EXPECT_EQ(get_move_source(move), e2);
    EXPECT_EQ(get_move_target(move), e4);
    EXPECT_EQ(get_move_piece(move), P);
    EXPECT_EQ(get_move_promoted(move), 0);
    EXPECT_EQ(get_move_capture(move), 0);
    EXPECT_EQ(get_move_double(move), 1 << 21);
    EXPECT_EQ(get_move_enpassant(move), 0);
    EXPECT_EQ(get_move_castling(move), 0);
}

TEST(MoveEncoding, CapturePromotionCastling) {
    const int move = encode_move(a1, h1, K, 0, 1, 0, 0, 1);
    EXPECT_TRUE(get_move_capture(move));
    EXPECT_TRUE(get_move_castling(move));

    const int promo = encode_move(a7, a8, P, Q, 0, 0, 0, 0);
    EXPECT_EQ(get_move_promoted(promo), Q);
}

// --- Square conversion (Board) ---

TEST(BoardSquareConversion, KnownSquares) {
    EXPECT_EQ(Board::sq_to_int("a1"), a1);
    EXPECT_EQ(Board::sq_to_int("e4"), e4);
    EXPECT_EQ(Board::sq_to_int("h8"), h8);
    EXPECT_EQ(Board::int_to_sq(e4), "e4");
}

TEST(BoardSquareConversion, AllSquaresRoundTrip) {
    for (int sq = a1; sq <= h8; ++sq) {
        const std::string algebraic = Board::int_to_sq(sq);
        EXPECT_EQ(Board::sq_to_int(algebraic), sq);
    }
}

// --- FEN parsing (Board::parseFEN) ---

TEST_F(EngineFixture, FenParsingStartPosition) {
    Board brd;
    brd.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    EXPECT_EQ(brd.side(), 0);
    EXPECT_EQ(brd.castle_rights(), 15);
    EXPECT_EQ(brd.en_passant(), -1);
    EXPECT_EQ(brd.half_move_clock(), 0);
    EXPECT_EQ(brd.full_move_counter(), 1);
    EXPECT_TRUE((brd.piece_on(K, e1)));
    EXPECT_TRUE((brd.piece_on(k, e8)));
    EXPECT_EQ((count_bits(brd.occupancy(0))), 16);
    EXPECT_EQ((count_bits(brd.occupancy(1))), 16);
}

TEST_F(EngineFixture, FenParsingBlackToMoveAndNoCastling) {
    Board brd;
    brd.parseFEN("8/8/8/8/4k3/8/8/4K3 b - - 50 120");

    EXPECT_EQ(brd.side(), 1);
    EXPECT_EQ(brd.castle_rights(), 0);
    EXPECT_EQ(brd.half_move_clock(), 50);
    EXPECT_EQ(brd.full_move_counter(), 120);
    EXPECT_TRUE((brd.piece_on(K, e1)));
    EXPECT_TRUE((brd.piece_on(k, e4)));
}

TEST_F(EngineFixture, FenParsingEnPassantSquare) {
    Board brd;
    brd.parseFEN("8/8/8/8/4P3/8/8/8 w - e3 0 1");

    EXPECT_EQ(brd.en_passant(), e3);
}

// --- EPD parser (epd.cpp) — isolated string parsing ---

TEST(EpdParser, ParsesBestMoveAndId) {
    EpdPosition pos;
    const std::string line =
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 "
        "bm d2d4 g2g4; id \"WAC001\";";

    ASSERT_TRUE(parse_epd_line(line, &pos));
    EXPECT_EQ(pos.fen, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    ASSERT_EQ(pos.best_moves.size(), 2u);
    EXPECT_EQ(pos.best_moves[0], "d2d4");
    EXPECT_EQ(pos.best_moves[1], "g2g4");
    EXPECT_EQ(pos.id, "WAC001");
}

TEST(EpdParser, RejectsCommentsAndMissingBestMove) {
    EpdPosition pos;
    EXPECT_FALSE(parse_epd_line("# comment", &pos));
    EXPECT_FALSE(parse_epd_line("", &pos));
    EXPECT_FALSE(parse_epd_line(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 id \"no bm\";",
        &pos));
    EXPECT_FALSE(parse_epd_line("not a fen at all", &pos));
    EXPECT_FALSE(parse_epd_line("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &pos));
    EXPECT_FALSE(parse_epd_line("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", nullptr));
}

// --- Zobrist hashing (tt.cpp) ---

TEST_F(EngineFixture, ZobristHashStableForSamePosition) {
    Board first;
    Board second;
    const char* fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    first.parseFEN(fen);
    second.parseFEN(fen);

    EXPECT_EQ(first.hash_key(), second.hash_key());
    EXPECT_EQ(first.hash_key(), compute_hash(&first));
}

TEST_F(EngineFixture, ZobristHashChangesWithSideToMove) {
    Board white_to_move;
    Board black_to_move;
    white_to_move.parseFEN("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
    black_to_move.parseFEN("4k3/8/8/8/8/8/8/4K3 b - - 0 1");

    EXPECT_NE(white_to_move.hash_key(), black_to_move.hash_key());
}

// --- Draw detection (draw.cpp) ---

TEST(DrawDetection, InsufficientMaterialKingsOnly) {
    Board brd;
    brd.parseFEN("4k3/8/8/8/8/8/8/4K3 w - - 0 1");

    EXPECT_TRUE(is_insufficient_material(&brd));
    EXPECT_FALSE(is_fifty_move_draw(&brd));
    EXPECT_TRUE(is_draw(&brd));
}

TEST(DrawDetection, InsufficientMaterialSameColorBishops) {
    Board brd;
    // White Bf2 and black Bb2 — bishops on the same square color.
    brd.parseFEN("4k3/8/8/8/8/1b3B3/8/4K3 w - - 0 1");

    EXPECT_TRUE(is_insufficient_material(&brd));
}

TEST(DrawDetection, SufficientMaterialWithPawn) {
    Board brd;
    brd.parseFEN("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1");

    EXPECT_FALSE(is_insufficient_material(&brd));
    EXPECT_FALSE(is_draw(&brd));
}

TEST(DrawDetection, FiftyMoveRule) {
    Board brd;
    brd.parseFEN("4k3/8/8/8/8/8/8/4K3 w - - 100 1");

    EXPECT_TRUE(is_fifty_move_draw(&brd));
    EXPECT_TRUE(is_draw(&brd));
}

// --- Transposition table (tt.cpp) ---

TEST_F(EngineFixture, TranspositionTableStoreAndProbeExact) {
    const U64 key = 0xDEADBEEFULL;
    const int stored_move = encode_move(e2, e4, P, 0, 0, 1, 0, 0);

    store_tt(key, 8, 0, 42, TT_EXACT, stored_move);

    int score = 0;
    int move = 0;
    Board board;
    board.init_start_position();
    const TTFlag flag = probe_tt(&board, key, 8, 0, -1000, 1000, &score, &move);

    EXPECT_EQ(flag, TT_EXACT);
    EXPECT_EQ(score, 42);
    EXPECT_EQ(move, stored_move);
    EXPECT_EQ(probe_tt_move(&board, key), stored_move);
}

TEST_F(EngineFixture, TranspositionTableProbeMissOnShallowDepth) {
    const U64 key = 0xCAFEBABEULL;
    store_tt(key, 4, 0, 10, TT_EXACT, 0);

    int score = 0;
    int move = 0;
    Board board;
    board.init_start_position();
    const TTFlag flag = probe_tt(&board, key, 6, 0, -1000, 1000, &score, &move);

    EXPECT_EQ(flag, TT_NONE);
    EXPECT_EQ(probe_tt_move(&board, key), 0);
}
