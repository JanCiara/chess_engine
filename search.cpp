#include <chrono>
#include <iostream>
#include "search.h"
#include "uci.h"
#include "tt.h"
#include "draw.h"

const int Search::piece_values[12] = {
    100, 300, 350, 500, 1000, 10000,
    100, 300, 350, 500, 1000, 10000
};

long long Search::now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

void Search::request_stop() {
    stop_search_ = true;
}

void Search::game_history_reset() {
    game_history_len_ = 0;
}

void Search::game_history_push(U64 key) {
    if (game_history_len_ < MAX_GAME_HISTORY) {
        game_history_[game_history_len_++] = key;
    }
}

int Search::allocate_time_ms(const Board* board, const SearchLimits& limits) const {
    if (limits.infinite) {
        return -1;
    }
    if (limits.movetime > 0) {
        return limits.movetime;
    }

    int time = (board->side() == WHITE) ? limits.wtime : limits.btime;
    int inc = (board->side() == WHITE) ? limits.winc : limits.binc;

    if (time <= 0) {
        return -1;
    }

    int allocation;
    if (limits.movestogo > 0) {
        allocation = time / limits.movestogo + inc * 3 / 4;
    } else {
        allocation = time / 25 + inc * 3 / 4;
    }

    if (allocation < 50) {
        allocation = 50;
    }
    if (allocation > time / 2) {
        allocation = time / 2;
    }
    if (allocation > time - 100) {
        allocation = time - 100;
    }
    if (allocation < 50) {
        allocation = 50;
    }

    return allocation;
}

void Search::check_time() {
    nodes_++;
    if (search_time_limit_ms_ < 0) {
        return;
    }
    if (nodes_ % 512 == 0) {
        if (now_ms() - search_start_ms_ >= search_time_limit_ms_ * 9 / 10) {
            stop_search_ = true;
        }
    }
}

int Search::in_check(const Board* board) {
    int king_sq = get_LSB(board->bitboard((board->side() == WHITE) ? K : k));
    return is_square_attacked(king_sq, board->side() ^ 1, board);
}

bool Search::has_non_pawn_material(const Board* board) {
    if (board->side() == WHITE) {
        return board->bitboard(N) || board->bitboard(B)
            || board->bitboard(R) || board->bitboard(Q);
    }
    return board->bitboard(n) || board->bitboard(b)
        || board->bitboard(r) || board->bitboard(q);
}

bool Search::is_repetition(U64 key) const {
    for (int i = 0; i < rep_ply_ - 1; i++) {
        if (rep_stack_[i] == key) {
            return true;
        }
    }
    return false;
}

int Search::draw_score(const Board* board) const {
    if (is_draw(board)) {
        return 0;
    }
    if (is_repetition(board->hash_key())) {
        return 0;
    }
    return -1;
}

int Search::get_victim_piece(const Board* board, int square) {
    int piece = board->piece_at(square);
    return piece >= 0 ? piece : 0;
}

static U64 see_attackers_to(int square, const U64 bb[12], U64 occupancy) {
    return (pawn_attacks[WHITE][square] & bb[p])
        | (pawn_attacks[BLACK][square] & bb[P])
        | (knight_attacks[square] & (bb[N] | bb[n]))
        | (king_attacks[square] & (bb[K] | bb[k]))
        | (get_bishop_attacks(square, occupancy) & (bb[B] | bb[b] | bb[Q] | bb[q]))
        | (get_rook_attacks(square, occupancy) & (bb[R] | bb[r] | bb[Q] | bb[q]));
}

static int see_find_attacker(int square, int side, const U64 bb[12], U64 occupancy, int* attacker_sq) {
    static const int white_order[] = {P, N, B, R, Q, K};
    static const int black_order[] = {p, n, b, r, q, k};
    const int* order = (side == WHITE) ? white_order : black_order;

    U64 attackers = see_attackers_to(square, bb, occupancy);
    U64 side_pieces = 0;
    for (int i = 0; i < 6; i++) {
        side_pieces |= bb[order[i]];
    }
    attackers &= side_pieces;

    for (int i = 0; i < 6; i++) {
        int piece = order[i];
        U64 piece_bb = bb[piece] & attackers;
        if (piece_bb) {
            *attacker_sq = get_LSB(piece_bb);
            return piece;
        }
    }
    return 0;
}

int Search::see(const Board* board, int move) {
    if (!get_move_capture(move) && !get_move_promoted(move)) {
        return 0;
    }

    int from = get_move_source(move);
    int to = get_move_target(move);
    int piece = get_move_piece(move);
    int promo = get_move_promoted(move);

    int captured_sq = to;
    int captured = get_victim_piece(board, to);
    if (get_move_enpassant(move)) {
        captured_sq = (board->side() == WHITE) ? to - 8 : to + 8;
        captured = (board->side() == WHITE) ? p : P;
    }

    if (captured == 0 && !promo) {
        return 0;
    }

    U64 bb[12];
    for (int i = P; i <= k; i++) {
        bb[i] = board->bitboard(i);
    }
    U64 occupancy = board->occupancy(2);

    pop_bit(bb[piece], from);
    pop_bit(occupancy, from);

    if (captured) {
        pop_bit(bb[captured], captured_sq);
        pop_bit(occupancy, captured_sq);
    }

    int placed = promo ? promo : piece;
    set_bit(bb[placed], to);
    set_bit(occupancy, to);

    int gain[32];
    int depth = 0;
    gain[0] = piece_values[captured];
    if (promo) {
        gain[0] += piece_values[promo] - piece_values[piece];
    }

    int side = board->side() ^ 1;

    while (depth < 31) {
        int attacker_sq = 0;
        int attacker = see_find_attacker(to, side, bb, occupancy, &attacker_sq);
        if (!attacker) {
            break;
        }

        depth++;
        gain[depth] = piece_values[attacker] - gain[depth - 1];
        if (gain[depth] < -gain[depth - 1]) {
            gain[depth] = -gain[depth - 1];
        }

        int victim = 0;
        for (int p = P; p <= k; p++) {
            if (get_bit(bb[p], to)) {
                victim = p;
                break;
            }
        }
        if (victim) {
            pop_bit(bb[victim], to);
        }
        pop_bit(bb[attacker], attacker_sq);
        pop_bit(occupancy, attacker_sq);

        set_bit(bb[attacker], to);
        set_bit(occupancy, to);

        side ^= 1;
    }

    while (depth > 0) {
        gain[depth - 1] = gain[depth - 1] > -gain[depth] ? gain[depth - 1] : -gain[depth];
        depth--;
    }

    return gain[0];
}

int Search::score_move(int move, const Board* board) const {
    if (move == follow_pv_move_) {
        return 3000000;
    }

    if (move == current_tt_move_) {
        return 2000000;
    }

    if (get_move_promoted(move) && !get_move_capture(move)) {
        return 1000000 + piece_values[get_move_promoted(move)];
    }

    if (get_move_capture(move) || get_move_promoted(move)) {
        return 1000000 + see(board, move);
    }

    return 0;
}

void Search::sort_moves(Moves* move_list, const Board* board) {
    struct ScoredMove {
        int move;
        int score;
    };

    ScoredMove scored[256];
    const int count = move_list->count;

    for (int i = 0; i < count; i++) {
        scored[i].move = move_list->moves[i];
        scored[i].score = score_move(move_list->moves[i], board);
    }

    for (int i = 1; i < count; i++) {
        ScoredMove key = scored[i];
        int j = i - 1;
        while (j >= 0 && scored[j].score < key.score) {
            scored[j + 1] = scored[j];
            j--;
        }
        scored[j + 1] = key;
    }

    for (int i = 0; i < count; i++) {
        move_list->moves[i] = scored[i].move;
    }
}

int Search::build_pv(const Board* board, int* pv, int max_len) const {
    Board temp = *board;
    int len = 0;

    if (best_move_ != 0) {
        pv[len++] = best_move_;
        Undo undo;
        if (!make_move(&temp, best_move_, &undo, 0)) {
            return len;
        }
    }

    while (len < max_len) {
        int tt_move = probe_tt_move(&temp, temp.hash_key());
        if (tt_move == 0) {
            break;
        }

        bool legal = false;
        Moves move_list[1];
        generate_moves(&temp, move_list);
        for (int i = 0; i < move_list->count; i++) {
            if (move_list->moves[i] == tt_move) {
                legal = true;
                break;
            }
        }
        if (!legal) {
            break;
        }

        pv[len++] = tt_move;
        Undo undo;
        if (!make_move(&temp, tt_move, &undo, 0)) {
            break;
        }
    }

    return len;
}

void Search::print_search_info(const Board* board, int depth, int score, long long elapsed) const {
    int pv[64];
    int pv_len = build_pv(board, pv, 64);

    std::cout << "info depth " << depth
              << " seldepth " << search_seldepth_;

    if (score >= MATE_SCORE - 256) {
        std::cout << " score mate " << (MATE_SCORE - score + 1) / 2;
    } else if (score <= -MATE_SCORE + 256) {
        std::cout << " score mate " << -((MATE_SCORE + score + 1) / 2);
    } else {
        std::cout << " score cp " << score;
    }

    std::cout << " nodes " << nodes_
              << " time " << elapsed;

    if (elapsed > 0) {
        std::cout << " nps " << (nodes_ * 1000LL / elapsed);
    }

    if (pv_len > 0) {
        std::cout << " pv";
        char uci[6];
        for (int i = 0; i < pv_len; i++) {
            move_to_uci(pv[i], uci);
            std::cout << " " << uci;
        }
    }

    std::cout << std::endl;
}

int Search::quiescence(int alpha, int beta, int ply, Board* board) {
    if (ply > search_seldepth_) {
        search_seldepth_ = ply;
    }

    check_time();
    if (stop_search_) {
        return 0;
    }

    int draw = draw_score(board);
    if (draw >= 0) {
        return draw;
    }

    int eval = evaluate(board);
    if (eval >= beta) {
        return beta;
    }
    if (eval > alpha) {
        alpha = eval;
    }

    Moves move_list[1];
    generate_moves(board, move_list);

    int check = in_check(board);
    sort_moves(move_list, board);

    int legal_moves = 0;

    for (int i = 0; i < move_list->count; i++) {
        if (stop_search_) {
            break;
        }

        int move = move_list->moves[i];
        if (!check && !get_move_capture(move) && !get_move_promoted(move)) {
            continue;
        }
        if (!check && get_move_capture(move) && see(board, move) < 0) {
            continue;
        }

        Undo undo;
        if (!make_move(board, move, &undo, 0)) {
            continue;
        }
        legal_moves++;

        rep_stack_[rep_ply_++] = board->hash_key();
        int score = -quiescence(-beta, -alpha, ply + 1, board);
        rep_ply_--;

        undo_move(board, &undo);

        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    if (legal_moves == 0 && check) {
        return -MATE_SCORE + ply;
    }

    return alpha;
}

int Search::compute_lmr(int depth, int moves_searched, int move, int check) {
    if (check) {
        return 0;
    }
    if (moves_searched == 0) {
        return 0;
    }
    if (depth < 3) {
        return 0;
    }
    if (moves_searched < 4) {
        return 0;
    }
    if (get_move_capture(move) || get_move_promoted(move)) {
        return 0;
    }

    int r = 1;
    if (depth >= 6 && moves_searched >= 10) {
        r = 2;
    }
    if (r > depth - 2) {
        r = depth - 2;
    }
    return r;
}

int Search::negamax(int alpha, int beta, int depth, int ply, Board* board) {
    if (ply > search_seldepth_) {
        search_seldepth_ = ply;
    }

    check_time();
    if (stop_search_) {
        return 0;
    }

    int draw = draw_score(board);
    if (draw >= 0) {
        return draw;
    }

    int check = in_check(board);
    if (check) {
        depth += CHECK_EXT;
    }

    if (depth <= 0) {
        return quiescence(alpha, beta, ply, board);
    }

    int tt_score = 0;
    TTFlag tt_flag = probe_tt(board, board->hash_key(), depth, ply, alpha, beta, &tt_score, &current_tt_move_);

    if (tt_flag == TT_EXACT) {
        return tt_score;
    }
    if (tt_flag == TT_ALPHA) {
        return tt_score;
    }
    if (tt_flag == TT_BETA) {
        return tt_score;
    }

    int null_depth = depth - 1 - NULL_MOVE_R;
    if (null_depth >= 1 && !check && has_non_pawn_material(board)) {
        Board::NullUndo null_undo;
        board->apply_null_move(&null_undo);
        rep_stack_[rep_ply_++] = board->hash_key();

        int null_score = -negamax(-beta, -beta + 1, null_depth, ply + 1, board);

        rep_ply_--;
        board->undo_null_move(&null_undo);

        if (null_score >= beta) {
            return beta;
        }
    }

    int alpha_orig = alpha;

    Moves move_list[1];
    generate_moves(board, move_list);
    sort_moves(move_list, board);

    int legal_moves = 0;
    int moves_searched = 0;
    int best_move_store = current_tt_move_;

    for (int i = 0; i < move_list->count; i++) {
        if (stop_search_) {
            break;
        }

        Undo undo;
        if (!make_move(board, move_list->moves[i], &undo, 0)) {
            continue;
        }
        legal_moves++;

        int move = move_list->moves[i];
        int reduction = compute_lmr(depth, moves_searched, move, check);
        int search_depth = depth - 1 - reduction;
        if (search_depth < 1) {
            search_depth = 1;
        }

        rep_stack_[rep_ply_++] = board->hash_key();

        int score;
        if (reduction > 0) {
            score = -negamax(-alpha - 1, -alpha, search_depth, ply + 1, board);
            if (score > alpha) {
                score = -negamax(-beta, -alpha, depth - 1, ply + 1, board);
            }
        } else if (moves_searched == 0) {
            score = -negamax(-beta, -alpha, depth - 1, ply + 1, board);
        } else {
            score = -negamax(-alpha - 1, -alpha, depth - 1, ply + 1, board);
            if (score > alpha && score < beta) {
                score = -negamax(-beta, -alpha, depth - 1, ply + 1, board);
            }
        }

        rep_ply_--;
        undo_move(board, &undo);
        moves_searched++;

        if (score > alpha) {
            alpha = score;
            best_move_store = move_list->moves[i];

            if (alpha >= beta) {
                store_tt(board->hash_key(), depth, ply, beta, TT_BETA, best_move_store);
                return beta;
            }
        }
    }

    if (legal_moves == 0) {
        int king_sq = get_LSB(board->bitboard((board->side() == WHITE) ? K : k));

        if (is_square_attacked(king_sq, board->side() ^ 1, board)) {
            return -MATE_SCORE + ply;
        }
        return 0;
    }

    TTFlag flag = TT_EXACT;
    if (alpha <= alpha_orig) {
        flag = TT_ALPHA;
    }

    store_tt(board->hash_key(), depth, ply, alpha, flag, best_move_store);

    return alpha;
}

int Search::search_root(Board* board, int depth, int alpha, int beta) {
    search_seldepth_ = 0;

    int best_score = -INF;
    int iteration_best_move = 0;

    Moves move_list[1];
    generate_moves(board, move_list);
    sort_moves(move_list, board);

    int legal_moves = 0;

    for (int i = 0; i < move_list->count; i++) {
        if (stop_search_) {
            break;
        }

        Undo undo;
        if (!make_move(board, move_list->moves[i], &undo, 0)) {
            continue;
        }
        legal_moves++;

        rep_stack_[rep_ply_++] = board->hash_key();
        int score = -negamax(-beta, -alpha, depth - 1, 1, board);
        rep_ply_--;

        undo_move(board, &undo);

        if (score > best_score) {
            best_score = score;
            iteration_best_move = move_list->moves[i];
        }

        if (score > alpha) {
            alpha = score;
            iteration_best_move = move_list->moves[i];
        }
    }

    if (legal_moves == 0) {
        if (in_check(board)) {
            return -MATE_SCORE;
        }
        return 0;
    }

    if (iteration_best_move != 0 && !stop_search_) {
        best_move_ = iteration_best_move;
    }

    return best_score;
}

int Search::search_root_aspiration(Board* board, int depth, int prev_score) {
    int alpha = -INF;
    int beta = INF;

    if (depth >= 2
        && prev_score > -MATE_SCORE + 256
        && prev_score < MATE_SCORE - 256) {
        alpha = prev_score - ASPIRATION_WINDOW;
        beta = prev_score + ASPIRATION_WINDOW;
    }

    while (true) {
        int score = search_root(board, depth, alpha, beta);
        if (score <= alpha) {
            if (alpha <= -INF + 1) {
                return score;
            }
            alpha = -INF;
            continue;
        }
        if (score >= beta) {
            if (beta >= INF - 1) {
                return score;
            }
            beta = INF;
            continue;
        }
        return score;
    }
}

bool Search::should_start_next_depth(int current_depth, long long elapsed_ms) const {
    if (search_time_limit_ms_ < 0) {
        return true;
    }

    if (elapsed_ms >= search_time_limit_ms_) {
        return false;
    }

    long long avg_per_depth = elapsed_ms / current_depth;
    return elapsed_ms + avg_per_depth <= search_time_limit_ms_ * 95 / 100;
}

void Search::search_position(Board* board, const SearchLimits& limits) {
    stop_search_ = false;
    follow_pv_move_ = 0;
    current_tt_move_ = 0;
    best_move_ = 0;
    nodes_ = 0;
    search_start_ms_ = now_ms();
    search_time_limit_ms_ = allocate_time_ms(board, limits);
    if (search_time_limit_ms_ < 0 && !limits.infinite && limits.depth == 0) {
        search_time_limit_ms_ = 1000;
    }

    rep_ply_ = 0;
    for (int i = 0; i < game_history_len_ && rep_ply_ < MAX_SEARCH_REP; i++) {
        rep_stack_[rep_ply_++] = game_history_[i];
    }

    SearchLimits effective = limits;
    if (effective.depth == 0 && search_time_limit_ms_ < 0 && !effective.infinite) {
        effective.depth = 6;
    }

    int max_depth = effective.depth > 0 ? effective.depth : 32;
    int last_score = 0;

    for (int depth = 1; depth <= max_depth; depth++) {
        if (stop_search_) {
            break;
        }

        last_score = search_root_aspiration(board, depth, last_score);

        if (!stop_search_ && best_move_ != 0) {
            follow_pv_move_ = best_move_;
        }

        long long elapsed = now_ms() - search_start_ms_;
        if (!limits.quiet) {
            print_search_info(board, depth, last_score, elapsed);
        }

        if (stop_search_) {
            break;
        }

        if (effective.depth > 0 && depth >= effective.depth) {
            break;
        }

        if (search_time_limit_ms_ >= 0 && elapsed >= search_time_limit_ms_) {
            break;
        }

        if (search_time_limit_ms_ >= 0 && !should_start_next_depth(depth, elapsed)) {
            break;
        }
    }

    if (limits.quiet) {
        return;
    }

    if (best_move_ == 0) {
        Moves move_list[1];
        generate_moves(board, move_list);
        if (move_list->count > 0) {
            best_move_ = move_list->moves[0];
        }
    }

    if (best_move_ == 0) {
        std::cout << "bestmove (none)" << std::endl;
        return;
    }

    char uci[6];
    move_to_uci(best_move_, uci);
    std::cout << "bestmove " << uci << std::endl;
}
