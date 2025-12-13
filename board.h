#pragma once
#include <string>
#include <unordered_map>
#include "defs.h"

class Board {
public:
    // 6 White, 6 Black
    U64 bitboards[12] = {0ULL};

    // [0] - White, [1] - Black, [2] - All
    U64 occupancies[3] = {0ULL};

    // 0 - White, 1 - Black
    int side = 0;

    // Represents the field (e.g. d7) where the en passant is possible
    int en_passant = -1; // -1 means no en passant possible

    // Bits representing the ability to castle
    // 1 (0001) - White king (WK) can short castle
    // 2 (0010) - White king (WQ) can long castle
    // 4 (0100) - White king (BK) can short castle
    // 8 (1000) - White king (BQ) can long castle
    int castle = 15;

    int full_move_counter = 0;
    int half_move_clock = 0;

    U64 KnightAttacks[64];
    U64 KingAttacks[64];
    U64 WhitePawnAttacks[64];
    U64 BlackPawnAttacks[64];

    static constexpr char ascii_pieces[12] = {'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k'};

    inline static const std::unordered_map<char, int> ascii_to_int = {
        {'P', P},
        {'N', N},
        {'B', B},
        {'R', R},
        {'Q', Q},
        {'K', K},
        {'p', p},
        {'n', n},
        {'b', b},
        {'r', r},
        {'q', q},
        {'k', k}
    };

    Board();

    void init_start_position();
    void update_occupancies();
    static void print_bitboard(U64 bb) ;
    void print_board() const;
    void parseFEN(const std::string &fen);

    // Helper functions
    static int sq_to_int(const std::string &square);
    static std::string int_to_sq(int square);

    // For generating knight moves
    static U64 noNoEa(U64 b);
    static U64 noEaEa(U64 b);
    static U64 soSoEa(U64 b);
    static U64 soEaEa(U64 b);
    
    static U64 noNoWe(U64 b);
    static U64 noWeWe(U64 b);
    static U64 soWeWe(U64 b);
    static U64 soSoWe(U64 b);

    // For generating king moves
    static U64 noOne(U64 b);
    static U64 eaOne(U64 b);
    static U64 weOne(U64 b);
    static U64 soOne(U64 b);

    static U64 noEaOne(U64 b);
    static U64 noWeOne(U64 b);
    static U64 soEaOne(U64 b);
    static U64 soWeOne(U64 b);

};
