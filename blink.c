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
    /* note that the maximum integer divider for clk_ref is 256 */
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

    scb_hw->scr |= M33_SCR_SLEEPDEEP_BITS;

    /* turn off clocks to a bunch of stuff we aren't using */
    clocks_hw->wake_en1 = (CLOCKS_WAKE_EN1_BITS &
                           ~(CLOCKS_WAKE_EN1_CLK_USB_BITS |
                             CLOCKS_WAKE_EN1_CLK_SYS_USBCTRL_BITS |
                             CLOCKS_WAKE_EN1_CLK_SYS_UART1_BITS |
                             CLOCKS_WAKE_EN1_CLK_PERI_UART1_BITS |
                             CLOCKS_WAKE_EN1_CLK_SYS_UART0_BITS |
                             CLOCKS_WAKE_EN1_CLK_PERI_UART0_BITS |
                             CLOCKS_WAKE_EN1_CLK_SYS_TRNG_BITS |
                             CLOCKS_WAKE_EN1_CLK_SYS_TIMER1_BITS |
                             CLOCKS_WAKE_EN1_CLK_SYS_TIMER0_BITS |
                             CLOCKS_WAKE_EN1_CLK_SYS_SPI1_BITS |
                             CLOCKS_WAKE_EN1_CLK_PERI_SPI1_BITS |
                             CLOCKS_WAKE_EN1_CLK_SYS_SPI0_BITS |
                             CLOCKS_WAKE_EN1_CLK_PERI_SPI0_BITS));

    clocks_hw->wake_en0 = (CLOCKS_WAKE_EN0_BITS &
                           ~(CLOCKS_WAKE_EN0_CLK_SYS_SHA256_BITS |
                             CLOCKS_WAKE_EN0_CLK_SYS_ROSC_BITS |
                             CLOCKS_WAKE_EN0_CLK_SYS_PWM_BITS |
                             CLOCKS_WAKE_EN0_CLK_SYS_PLL_USB_BITS |
                             CLOCKS_WAKE_EN0_CLK_SYS_PIO2_BITS |
                             CLOCKS_WAKE_EN0_CLK_SYS_PIO1_BITS |
                             CLOCKS_WAKE_EN0_CLK_SYS_PIO0_BITS |
                             CLOCKS_WAKE_EN0_CLK_SYS_JTAG_BITS |
                             CLOCKS_WAKE_EN0_CLK_SYS_I2C1_BITS |
                             CLOCKS_WAKE_EN0_CLK_SYS_I2C0_BITS |
                             CLOCKS_WAKE_EN0_CLK_SYS_HSTX_BITS |
                             CLOCKS_WAKE_EN0_CLK_HSTX_BITS |
                             CLOCKS_WAKE_EN0_CLK_SYS_DMA_BITS |
                             CLOCKS_WAKE_EN0_CLK_SYS_ADC_BITS |
                             CLOCKS_WAKE_EN0_CLK_ADC_BITS));

    /* make sure we don't clock anything in sleep that wasn't clocked in wake */
    clocks_hw->sleep_en1 = (clocks_hw->wake_en1 &
                            ~(CLOCKS_SLEEP_EN1_CLK_SYS_SYSCFG_BITS));
    clocks_hw->sleep_en0 = (clocks_hw->wake_en0 &
                            ~(CLOCKS_SLEEP_EN0_CLK_REF_OTP_BITS |
                              CLOCKS_SLEEP_EN0_CLK_SYS_OTP_BITS |
                              CLOCKS_SLEEP_EN0_CLK_SYS_RESETS_BITS));

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
