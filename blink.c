#include "hardware/gpio.h"
#include "hardware/xosc.h"
#include "hardware/structs/rosc.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/pll.h"
#include "hardware/powman.h"

#include "RP2350.h"

#define ALARM_NUM 0

void yield(void) {
    /* we could do context switching here for cooperative multitasking if we wanted */
    __dsb();
    __wfe();
}

void run_from_xosc(void) {
    clock_configure_int_divider(clk_ref, CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC, 0, XOSC_MHZ * MHZ, 12);
    clock_configure_undivided(clk_sys, CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF, 0, XOSC_MHZ * MHZ / 12);

    clock_stop(clk_usb);
    clock_stop(clk_adc);
    clock_stop(clk_hstx);

    clock_configure_undivided(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS, XOSC_MHZ * MHZ / 12);

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
                            CLOCKS_SLEEP_EN0_CLK_SYS_POWMAN_BITS |
                            CLOCKS_SLEEP_EN0_CLK_REF_POWMAN_BITS |
                            CLOCKS_SLEEP_EN0_CLK_SYS_BUSFABRIC_BITS);

    clocks_hw->sleep_en1 = (CLOCKS_SLEEP_EN1_CLK_SYS_XIP_BITS |
                            CLOCKS_SLEEP_EN1_CLK_REF_TICKS_BITS |
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

    /* this is a misnomer, "using xosc" means using it via clk_ref */
    powman_timer_set_1khz_tick_source_xosc_with_hz(clock_get_hz(clk_ref));
    powman_timer_start();

    /* time intervals in microseconds for blink pattern */
    const unsigned period_ms = 2000, on_ms = 30;

    uint64_t blink_timer_next = powman_timer_get_ms();
    powman_enable_alarm_wakeup_at_ms(blink_timer_next + period_ms);

    irq_set_enabled(POWMAN_IRQ_TIMER, false);

    for (unsigned alarms = 0;; alarms++) {
        /* repeatedly sleep, until timer interrupt becomes pending */
        while (!(powman_hw->intr & POWMAN_INTR_TIMER_BITS))
            yield();

        /* acknowledge and clear interrupt in timer and nvic */
        powman_timer_disable_alarm();
        powman_clear_alarm();
        irq_clear(POWMAN_IRQ_TIMER);

        if (!(alarms % 2)) {
            /* turn on LED */
            gpio_put(PICO_DEFAULT_LED_PIN, 1);

            /* increment and rearm */
            blink_timer_next += on_ms;
        } else {
            /* turn off LED */
            gpio_put(PICO_DEFAULT_LED_PIN, 0);

            /* increment and rearm */
            blink_timer_next += period_ms - on_ms;
        }

        powman_enable_alarm_wakeup_at_ms(blink_timer_next);
    }
}
