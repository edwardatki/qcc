// Test extern variables

#include "io.h"

// heap_start label is inserted by the compiler at 
// the end of generated code. Handy for this test
extern void heap_start;

char main() {
    print("Heap starts at ");
    print_hex_u16(&heap_start);
    print("\n");

    return 0;
}