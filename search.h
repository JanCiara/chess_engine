#pragma once
#include "board.h"
#include "movegen.h"
#include "evaluate.h"

struct SearchLimits {
    int depth = 0;
    int movetime = 0;
    int wtime = 0;
    int btime = 0;
    int winc = 0;
    int binc = 0;
    int movestogo = 0;
    bool infinite = false;
    bool quiet = false;
};

class Search {
public:
    void search_position(Board* board, const SearchLimits& limits);
    void request_stop();
    long long nodes() const { return nodes_; }
    int best_move() const { return best_move_; }

    void game_history_reset();
    void game_history_push(U64 key);

private:
    static constexpr int INF = 50000;
    static constexpr int MATE_SCORE = 49000;
    static constexpr int MAX_GAME_HISTORY = 1024;
    static constexpr int MAX_SEARCH_REP = 2048;
    static constexpr int NULL_MOVE_R = 3;
    static constexpr int CHECK_EXT = 1;
    static constexpr int ASPIRATION_WINDOW = 50;

    static const int piece_values[12];

    int best_move_ = 0;
    U64 game_history_[MAX_GAME_HISTORY];
    int game_history_len_ = 0;
    U64 rep_stack_[MAX_SEARCH_REP];
    int rep_ply_ = 0;
    int search_seldepth_ = 0;
    bool stop_search_ = false;
    int follow_pv_move_ = 0;
    int current_tt_move_ = 0;
    long long nodes_ = 0;
    long long search_start_ms_ = 0;
    int search_time_limit_ms_ = -1;

    static long long now_ms();
    int allocate_time_ms(const Board* board, const SearchLimits& limits) const;
    void check_time();
    static int in_check(const Board* board);
    static bool has_non_pawn_material(const Board* board);
    bool is_repetition(U64 key) const;
    int draw_score(const Board* board) const;
    static int get_victim_piece(const Board* board, int square);
    static int see(const Board* board, int move);
    int score_move(int move, const Board* board) const;
    void sort_moves(Moves* move_list, const Board* board);
    int build_pv(const Board* board, int* pv, int max_len) const;
    void print_search_info(const Board* board, int depth, int score, long long elapsed) const;
    int quiescence(int alpha, int beta, int ply, Board* board);
    static int compute_lmr(int depth, int moves_searched, int move, int check);
    int negamax(int alpha, int beta, int depth, int ply, Board* board);
    int search_root(Board* board, int depth, int alpha, int beta);
    int search_root_aspiration(Board* board, int depth, int prev_score);
    bool should_start_next_depth(int current_depth, long long elapsed_ms) const;
};
