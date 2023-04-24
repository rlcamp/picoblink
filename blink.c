#include "pico/stdlib.h"

int main() {
    /* 25 for pico, 13 for adafruit feather rp2040 */
    const unsigned pin = 13;

    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);

    while (true) {
        gpio_put(pin, 1);
        sleep_ms(500);

        gpio_put(pin, 0);
        sleep_ms(250);
    }
}
