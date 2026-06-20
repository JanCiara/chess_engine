#include "draw.h"

static int count_pieces(const Board* board, int piece) {
    return count_bits(board->bitboard(piece));
}

static int bishop_color(int square) {
    return (square / 8 + square % 8) & 1;
}

bool is_insufficient_material(const Board* board) {
    if (board->bitboard(P) || board->bitboard(p)) {
        return false;
    }
    if (board->bitboard(R) || board->bitboard(r) || board->bitboard(Q) || board->bitboard(q)) {
        return false;
    }

    int white_minors = count_pieces(board, N) + count_pieces(board, B);
    int black_minors = count_pieces(board, n) + count_pieces(board, b);

    if (white_minors == 0 && black_minors == 0) {
        return true;
    }
    if (white_minors == 0 && black_minors == 1) {
        return true;
    }
    if (black_minors == 0 && white_minors == 1) {
        return true;
    }

    if (white_minors == 1 && black_minors == 1
        && count_pieces(board, B) == 1 && count_pieces(board, b) == 1
        && !board->bitboard(N) && !board->bitboard(n)) {
        int white_bishop = get_LSB(board->bitboard(B));
        int black_bishop = get_LSB(board->bitboard(b));
        return bishop_color(white_bishop) == bishop_color(black_bishop);
    }

    return false;
}

bool is_fifty_move_draw(const Board* board) {
    return board->half_move_clock() >= 100;
}

bool is_draw(const Board* board) {
    return is_fifty_move_draw(board) || is_insufficient_material(board);
}
