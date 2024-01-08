#include "mem.h"
#include "io.h"

char main() {
    char* p;
    char size;

    print("Allocate 8 bytes...\n");
    size = malloc(&p, 8);
    print_hex_u8(size);
    print(" bytes at ");
    print_hex_u16(p);
    print("\n");
    if (size < 8) return 1;

    print("Allocate 8 bytes...\n");
    size = malloc(&p, 8);
    print_hex_u8(size);
    print(" bytes at ");
    print_hex_u16(p);
    print("\n");
    if (size < 8) return 1;

    return 0;
}