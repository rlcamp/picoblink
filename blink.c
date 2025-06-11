#include "pico/stdlib.h"
#include "hardware/xosc.h"
#include "hardware/structs/rosc.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/pll.h"
#include "RP2350.h"

#define ALARM_NUM 0

void yield(void) {
    /* we could do context switching here for cooperative multitasking if we wanted */
    __dsb();
    __wfe();
}

void run_from_xosc(void) {
    clock_configure_undivided(clk_ref, CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC, 0, XOSC_MHZ * MHZ);
    clock_configure_undivided(clk_sys, CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF, 0, XOSC_MHZ * MHZ);

    clock_stop(clk_usb);
    clock_stop(clk_adc);
    clock_stop(clk_hstx);

    clock_configure_undivided(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS, XOSC_MHZ * MHZ);

    /* disable PLLs */
    pll_deinit(pll_sys);
    pll_deinit(pll_usb);

    /* disable rosc and wait for it to be stopped */
    rosc_hw->ctrl = (rosc_hw->ctrl & ~ROSC_CTRL_ENABLE_BITS) | (ROSC_CTRL_ENABLE_VALUE_DISABLE << ROSC_CTRL_ENABLE_LSB);
    while (rosc_hw->status & ROSC_STATUS_STABLE_BITS);
}

int main() {
    /* enable sevonpend so that we don't need a nearly empty isr */
    scb_hw->scr |= M33_SCR_SEVONPEND_BITS;

    /* this is not a cpu intensive application, just run at 12 MHz */
    run_from_xosc();

    /* disable systick */
    SysTick->CTRL = 0;

    scb_hw->scr |= M33_SCR_SLEEPDEEP_BITS;

    /* TODO: figure out if this combination of things is stable */
    clocks_hw->sleep_en0 = (CLOCKS_SLEEP_EN0_CLK_SYS_SIO_BITS |
                            CLOCKS_SLEEP_EN0_CLK_SYS_PADS_BITS |
                            CLOCKS_SLEEP_EN0_CLK_SYS_IO_BITS |
                            CLOCKS_SLEEP_EN0_CLK_SYS_BUSFABRIC_BITS);

    clocks_hw->sleep_en1 = (CLOCKS_SLEEP_EN1_CLK_SYS_XIP_BITS |
                            CLOCKS_SLEEP_EN1_CLK_REF_TICKS_BITS |
                            CLOCKS_SLEEP_EN1_CLK_SYS_TIMER0_BITS |
                            CLOCKS_SLEEP_EN1_CLK_SYS_SRAM9_BITS |
                            CLOCKS_SLEEP_EN1_CLK_SYS_SRAM8_BITS |
                            CLOCKS_SLEEP_EN1_CLK_SYS_SRAM7_BITS |
                            CLOCKS_SLEEP_EN1_CLK_SYS_SRAM6_BITS |
                            CLOCKS_SLEEP_EN1_CLK_SYS_SRAM5_BITS |
                            CLOCKS_SLEEP_EN1_CLK_SYS_SRAM4_BITS |
                            CLOCKS_SLEEP_EN1_CLK_SYS_SRAM3_BITS |
                            CLOCKS_SLEEP_EN1_CLK_SYS_SRAM2_BITS |
                            CLOCKS_SLEEP_EN1_CLK_SYS_SRAM1_BITS |
                            CLOCKS_SLEEP_EN1_CLK_SYS_SRAM0_BITS);


    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    /* enable interrupt for alarm in peripheral, but not in nvic */
    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
    irq_set_enabled(hardware_alarm_get_irq_num(ALARM_NUM), false);

    /* time intervals in microseconds for blink pattern */
    const unsigned period_us = 2000000, on_us = 30000;

    /* first tick will be one period from now */
    timer_hw->alarm[ALARM_NUM] = timer_hw->timerawl + period_us;

    for (unsigned alarms = 0;; alarms++) {
        /* repeatedly sleep, until timer interrupt becomes pending */
        while (!(timer_hw->intr & (1U << ALARM_NUM)))
            yield();

        /* acknowledge and clear interrupt in timer and nvic */
        hw_clear_bits(&timer_hw->intr, 1U << ALARM_NUM);
        irq_clear(hardware_alarm_get_irq_num(ALARM_NUM));

        if (!(alarms % 2)) {
            /* turn on LED */
            gpio_put(PICO_DEFAULT_LED_PIN, 1);

            /* increment and rearm */
            timer_hw->alarm[ALARM_NUM] += on_us;
        } else {
            /* turn off LED */
            gpio_put(PICO_DEFAULT_LED_PIN, 0);

            /* increment and rearm */
            timer_hw->alarm[ALARM_NUM] += period_us - on_us;
        }
    }
}
