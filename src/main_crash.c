#include <stdint.h>

void cause_hardfault(void)
{
    uint32_t * cause_hardfault = (uint32_t *) 0x1;

    uint32_t value = *cause_hardfault;
    (void) value;
}

int do_something(int unused) {
    cause_hardfault();
    return 1;
}

int main() {
    do_something(0);
    return 0;
}

