#include "board.h"
#include "movegen.h"
#include "tt.h"
#include "uci.h"

int main() {
    init_zobrist();
    uci_loop();

    return 0;
}