// Test 16 bit operations

// #include "io.h"

char main() {
    int x = 0x1234;
    int y = 0x4321;
    int z = 0x1111;

    x = x + y - z;

    // print_hex_u16(x);
    // print("\n");

    if (x != 0x4444) return 1;
    // if (x > 0x4444) return 2;
    // if (x < 0x4444) return 3;
    // if (x >= 4445) return 4;
    // if (x <= 0x4444) return 5;
    if (x == 0x4444) return 0;

    return 6;
}