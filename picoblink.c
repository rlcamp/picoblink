#include "pico/stdlib.h"
#include "RP2040.h"

static unsigned long ticks = 0;

void SysTick_Handler(void) {
    ticks++;
}

static int one_interval_has_elapsed(unsigned long interval_ticks) {
    static unsigned long prev;
    if (ticks - prev < interval_ticks) {
        __WFI();
        return 0;
    }
    prev += interval_ticks;
    return 1;
}

int main() {
    /* 25 for pico, 13 for adafruit feather rp2040 */
    static const unsigned pin = PICO_DEFAULT_LED_PIN;

    set_sys_clock_48mhz();
    SysTick_Config(12000000UL);

    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);

    while (true) {
        gpio_put(pin, 1);
        while (!one_interval_has_elapsed(2)) continue;

        gpio_put(pin, 0);
        while (!one_interval_has_elapsed(1)) continue;
    }
}
