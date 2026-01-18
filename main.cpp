#include "board.h"
#include "movegen.h"
#include "uci.h"

int main() {
    init_all_attacks();
    uci_loop();

    return 0;
}