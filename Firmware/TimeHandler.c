#include <stdint.h>
#include <stdbool.h>

volatile uint32_t milliseconds;

void millisHandler() {
    milliseconds++;
}

uint32_t millis() {
    return milliseconds;
}
