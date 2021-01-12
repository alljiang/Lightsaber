
#include <stdint.h>
#include <stdbool.h>

long milliseconds;

void millisHandler() {
    milliseconds++;
}

long millis() {
    return milliseconds;
}
