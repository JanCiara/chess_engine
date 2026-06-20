#include <chrono>
#include <iostream>
#include <string>
#include "search.h"
#include "tt.h"
#include "draw.h"

int best_move = 0;

#define INF 50000
#define MATE_SCORE 49000
#define MAX_GAME_HISTORY 1024
#define MAX_SEARCH_REP 2048

// Positions actually reached in the played game, used to seed repetition
// detection at the root.
static U64 game_history[MAX_GAME_HISTORY];
static int game_history_len = 0;

static U64 rep_stack[MAX_SEARCH_REP];
static int rep_ply = 0;
static int search_seldepth = 0;
static const int piece_values[12] = {
    100, 300, 350, 500, 1000, 10000,
    100, 300, 350, 500, 1000, 10000
};

static bool stop_search = false;
static int follow_pv_move = 0;
static int current_tt_move = 0;
static long long nodes = 0;
static long long search_start_ms = 0;
static int search_time_limit_ms = -1;

static long long now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

void request_stop() {
    stop_search = true;
}

void game_history_reset() {
    game_history_len = 0;
}

void game_history_push(U64 key) {
    if (game_history_len < MAX_GAME_HISTORY) {
        game_history[game_history_len++] = key;
    }
}

static int allocate_time_ms(const Board* board, const SearchLimits& limits) {
    if (limits.infinite) {
        return -1;
    }
    if (limits.movetime > 0) {
        return limits.movetime;
    }

    int time = (board->side == WHITE) ? limits.wtime : limits.btime;
    int inc = (board->side == WHITE) ? limits.winc : limits.binc;

    if (time <= 0) {
        return -1;
    }

    int allocation;
    if (limits.movestogo > 0) {
        allocation = time / limits.movestogo + inc * 3 / 4;
    } else {
        allocation = time / 20 + inc;
    }

    if (allocation > time / 2) {
        allocation = time / 2;
    }

    return allocation;
}

static void check_time() {
    nodes++;
    if (search_time_limit_ms < 0) {
        return;
    }
    if (nodes % 2048 == 0) {
        if (now_ms() - search_start_ms >= search_time_limit_ms) {
            stop_search = true;
        }
    }
}

static int in_check(const Board* board) {
    int king_sq = get_LSB(board->bitboards[(board->side == WHITE) ? K : k]);
    return is_square_attacked(king_sq, board->side ^ 1, board);
}

static bool has_non_pawn_material(const Board* board) {
    if (board->side == WHITE) {
        return board->bitboards[N] || board->bitboards[B]
            || board->bitboards[R] || board->bitboards[Q];
    }
    return board->bitboards[n] || board->bitboards[b]
        || board->bitboards[r] || board->bitboards[q];
}

static void apply_null_move(Board* board) {
    board->side ^= 1;
    board->en_passant = -1;
    board->hash_key = compute_hash(board);
}

static bool is_repetition(U64 key) {
    // rep_stack already contains the current position (pushed by the parent
    // before recursing), so a single additional match means the position has
    // occurred earlier in the line: treat that twofold as a draw in the tree.
    for (int i = 0; i < rep_ply - 1; i++) {
        if (rep_stack[i] == key) {
            return true;
        }
    }
    return false;
}

static int draw_score(const Board* board) {
    if (is_draw(board)) {
        return 0;
    }
    if (is_repetition(board->hash_key)) {
        return 0;
    }
    return -1;
}

static int get_victim_piece(const Board* board, int square) {
    for (int piece = P; piece <= k; piece++) {
        if (get_bit(board->bitboards[piece], square)) {
            return piece;
        }
    }
    return 0;
}

static int score_move(int move, const Board* board) {
    if (move == follow_pv_move) {
        return 3000000;
    }

    if (move == current_tt_move) {
        return 2000000;
    }

    if (get_move_promoted(move)) {
        return 1000000 + piece_values[get_move_promoted(move)];
    }

    if (!get_move_capture(move)) {
        return 0;
    }

    int victim = get_victim_piece(board, get_move_target(move));
    int attacker = get_move_piece(move);
    return 10 * piece_values[victim] - piece_values[attacker];
}

static void sort_moves(Moves* move_list, const Board* board) {
    for (int i = 0; i < move_list->count - 1; i++) {
        for (int j = i + 1; j < move_list->count; j++) {
            int score_i = score_move(move_list->moves[i], board);
            int score_j = score_move(move_list->moves[j], board);
            if (score_j > score_i) {
                int temp = move_list->moves[i];
                move_list->moves[i] = move_list->moves[j];
                move_list->moves[j] = temp;
            }
        }
    }
}

static std::string move_to_uci(int move) {
    std::string uci = Board::int_to_sq(get_move_source(move))
                    + Board::int_to_sq(get_move_target(move));

    int promo = get_move_promoted(move);
    if (promo) {
        char c = 'q';
        if (promo == R || promo == r) c = 'r';
        else if (promo == B || promo == b) c = 'b';
        else if (promo == N || promo == n) c = 'n';
        uci += c;
    }

    return uci;
}

static int build_pv(const Board* board, int* pv, int max_len) {
    Board temp = *board;
    int len = 0;

    if (best_move != 0) {
        pv[len++] = best_move;
        if (!make_move(&temp, best_move, 0)) {
            return len;
        }
    }

    while (len < max_len) {
        int tt_move = probe_tt_move(temp.hash_key);
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
        if (!make_move(&temp, tt_move, 0)) {
            break;
        }
    }

    return len;
}

static void print_search_info(const Board* board, int depth, int score, long long elapsed) {
    int pv[64];
    int pv_len = build_pv(board, pv, 64);

    std::cout << "info depth " << depth
              << " seldepth " << search_seldepth;

    if (score >= MATE_SCORE - 256) {
        std::cout << " score mate " << (MATE_SCORE - score + 1) / 2;
    } else if (score <= -MATE_SCORE + 256) {
        std::cout << " score mate " << -((MATE_SCORE + score + 1) / 2);
    } else {
        std::cout << " score cp " << score;
    }

    std::cout << " nodes " << nodes
              << " time " << elapsed;

    if (elapsed > 0) {
        std::cout << " nps " << (nodes * 1000LL / elapsed);
    }

    if (pv_len > 0) {
        std::cout << " pv";
        for (int i = 0; i < pv_len; i++) {
            std::cout << " " << move_to_uci(pv[i]);
        }
    }

    std::cout << std::endl;
}

static int quiescence(int alpha, int beta, int ply, Board* board) {
    if (ply > search_seldepth) {
        search_seldepth = ply;
    }

    check_time();
    if (stop_search) {
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
        if (stop_search) {
            break;
        }

        int move = move_list->moves[i];
        if (!check && !get_move_capture(move) && !get_move_promoted(move)) {
            continue;
        }

        Board copy = *board;
        if (!make_move(board, move, 0)) {
            continue;
        }
        legal_moves++;

        rep_stack[rep_ply++] = board->hash_key;
        int score = -quiescence(-beta, -alpha, ply + 1, board);
        rep_ply--;

        *board = copy;

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

#define NULL_MOVE_R 3

static int compute_lmr(int depth, int moves_searched, int move, int check) {
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

static int negamax(int alpha, int beta, int depth, int ply, Board* board) {
    if (ply > search_seldepth) {
        search_seldepth = ply;
    }

    check_time();
    if (stop_search) {
        return 0;
    }

    int draw = draw_score(board);
    if (draw >= 0) {
        return draw;
    }

    if (depth <= 0) {
        return quiescence(alpha, beta, ply, board);
    }

    int tt_score = 0;
    TTFlag tt_flag = probe_tt(board->hash_key, depth, ply, alpha, beta, &tt_score, &current_tt_move);

    if (tt_flag == TT_EXACT) {
        return tt_score;
    }
    if (tt_flag == TT_ALPHA) {
        return tt_score;
    }
    if (tt_flag == TT_BETA) {
        return tt_score;
    }

    // Null move pruning: if passing the turn still fails high, cut off.
    int null_depth = depth - 1 - NULL_MOVE_R;
    if (null_depth >= 1 && !in_check(board) && has_non_pawn_material(board)) {
        Board copy = *board;
        apply_null_move(board);
        rep_stack[rep_ply++] = board->hash_key;

        int null_score = -negamax(-beta, -beta + 1, null_depth, ply + 1, board);

        rep_ply--;
        *board = copy;

        if (null_score >= beta) {
            return beta;
        }
    }

    int alpha_orig = alpha;

    Moves move_list[1];
    generate_moves(board, move_list);
    sort_moves(move_list, board);

    int check = in_check(board);
    int legal_moves = 0;
    int moves_searched = 0;
    int best_move_store = current_tt_move;

    for (int i = 0; i < move_list->count; i++) {
        if (stop_search) {
            break;
        }

        Board copy = *board;

        if (!make_move(board, move_list->moves[i], 0)) {
            continue;
        }
        legal_moves++;

        int move = move_list->moves[i];
        int reduction = compute_lmr(depth, moves_searched, move, check);
        int search_depth = depth - 1 - reduction;
        if (search_depth < 1) {
            search_depth = 1;
        }

        rep_stack[rep_ply++] = board->hash_key;

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

        rep_ply--;
        *board = copy;
        moves_searched++;

        if (score > alpha) {
            alpha = score;
            best_move_store = move_list->moves[i];

            if (alpha >= beta) {
                store_tt(board->hash_key, depth, ply, beta, TT_BETA, best_move_store);
                return beta;
            }
        }
    }

    if (legal_moves == 0) {
        int king_sq = get_LSB(board->bitboards[(board->side == WHITE) ? K : k]);

        if (is_square_attacked(king_sq, board->side ^ 1, board)) {
            return -MATE_SCORE + ply;
        } else {
            return 0;
        }
    }

    TTFlag flag = TT_EXACT;
    if (alpha <= alpha_orig) {
        flag = TT_ALPHA;
    }

    store_tt(board->hash_key, depth, ply, alpha, flag, best_move_store);

    return alpha;
}

static int search_root(Board* board, int depth) {
    search_seldepth = 0;

    int alpha = -INF;
    int beta = INF;
    int best_score = -INF;
    int iteration_best_move = 0;

    Moves move_list[1];
    generate_moves(board, move_list);
    sort_moves(move_list, board);

    for (int i = 0; i < move_list->count; i++) {
        if (stop_search) {
            break;
        }

        Board copy = *board;

        if (!make_move(board, move_list->moves[i], 0)) {
            continue;
        }

        rep_stack[rep_ply++] = board->hash_key;
        int score = -negamax(-beta, -alpha, depth - 1, 1, board);
        rep_ply--;

        *board = copy;

        if (score > best_score) {
            best_score = score;
            iteration_best_move = move_list->moves[i];
        }

        if (score > alpha) {
            alpha = score;
            iteration_best_move = move_list->moves[i];
        }
    }

    if (iteration_best_move != 0 && !stop_search) {
        best_move = iteration_best_move;
    }

    return best_score;
}

static bool should_start_next_depth(int current_depth, long long elapsed_ms) {
    if (search_time_limit_ms < 0) {
        return true;
    }

    if (elapsed_ms >= search_time_limit_ms) {
        return false;
    }

    long long avg_per_depth = elapsed_ms / current_depth;
    return elapsed_ms + avg_per_depth <= search_time_limit_ms * 95 / 100;
}

void search_position(Board* board, const SearchLimits& limits) {
    stop_search = false;
    follow_pv_move = 0;
    current_tt_move = 0;
    best_move = 0;
    nodes = 0;
    search_start_ms = now_ms();
    search_time_limit_ms = allocate_time_ms(board, limits);

    rep_ply = 0;
    for (int i = 0; i < game_history_len && rep_ply < MAX_SEARCH_REP; i++) {
        rep_stack[rep_ply++] = game_history[i];
    }

    SearchLimits effective = limits;
    if (effective.depth == 0 && search_time_limit_ms < 0 && !effective.infinite) {
        effective.depth = 6;
    }

    int max_depth = effective.depth > 0 ? effective.depth : 64;
    int last_score = 0;

    for (int depth = 1; depth <= max_depth; depth++) {
        if (stop_search) {
            break;
        }

        last_score = search_root(board, depth);

        if (!stop_search && best_move != 0) {
            follow_pv_move = best_move;
        }

        long long elapsed = now_ms() - search_start_ms;
        print_search_info(board, depth, last_score, elapsed);

        if (stop_search) {
            break;
        }

        if (effective.depth > 0 && depth >= effective.depth) {
            break;
        }

        if (search_time_limit_ms >= 0 && elapsed >= search_time_limit_ms) {
            break;
        }

        if (search_time_limit_ms >= 0 && !should_start_next_depth(depth, elapsed)) {
            break;
        }
    }

    if (best_move == 0) {
        std::cout << "bestmove (none)" << std::endl;
        return;
    }

    std::cout << "bestmove " << move_to_uci(best_move) << std::endl;
}
