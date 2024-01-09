// Test 16 bit operations

#include "io.h"

char main() {
    int x = 1234;
    int y = 4321;
    int z = 1111;

    x = x + y - z;

    print_hex_u16(x);
    print("\n");

    if (x != 4444) return 1;
    if (x == 4444) return 0;

    return 2;
}