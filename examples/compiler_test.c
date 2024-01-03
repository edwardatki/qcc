#include "io.h"

char main() {
    print("C compiler test\n");
    
    char i = 0;
    while (i < 10) {
        char c = getc();
        if (c != 0) {
            i = i + 1;
            print_hex_u8(i);
            print("\n");
        }
    }

    print("Done\n");

    return 0;
}