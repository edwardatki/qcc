char first_run = 1;
void* heap_pointer;

char malloc(void** p, char size) {
    if (first_run) {
        extern void heap_start;
        heap_pointer = &heap_start;
        first_run = 0;
    }

    char* h = &heap_pointer;

    char* p2 = p;
    *p2 = *h;
    p2++;
    h++;
    *p2 = *h;

    // heap = heap + size
    char i = size;
    while (i--) heap_pointer++;

    return size;
}