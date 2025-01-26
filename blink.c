#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/pll.h"
#include "RP2350.h"

#define LED_DELAY_US 250000

#define ALARM_NUM 0

/* not volatile, because we don't want to obligate ISRs to make volatile accesses to it,
 and we can still enforce volatile access to it within main loop */
static unsigned alarms = 0;

static void alarm_irq(void) {
    /* acknowledge interrupt */
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);

    /* nonvolatile increment of number of alarms that have happened */
    alarms++;

    /* increment and rearm */
    timer_hw->alarm[ALARM_NUM] += LED_DELAY_US;
}

void yield(void) {
    /* we could do context switching here for cooperative multitasking if we wanted */
    __dsb();
    __wfe();
}

int main() {
    /* this is not a cpu intensive application, just run at 48 MHz */
    set_sys_clock_48mhz();

    /* disable systick */
    SysTick->CTRL = 0;

    /* while asleep, only clock the timer */
    scb_hw->scr |= M33_SCR_SLEEPDEEP_BITS;

    /* TODO: downselect these further */
    clocks_hw->sleep_en0 = (CLOCKS_SLEEP_EN0_CLK_SYS_SIO_BITS |
                            CLOCKS_SLEEP_EN0_CLK_SYS_PSM_BITS |
                            CLOCKS_SLEEP_EN0_CLK_SYS_RESETS_BITS |
                            CLOCKS_SLEEP_EN0_CLK_SYS_PADS_BITS |
                            CLOCKS_SLEEP_EN0_CLK_SYS_IO_BITS |
                            CLOCKS_SLEEP_EN0_CLK_SYS_BUSFABRIC_BITS);

    clocks_hw->sleep_en1 = (CLOCKS_SLEEP_EN1_CLK_SYS_TIMER1_BITS |
                            CLOCKS_SLEEP_EN1_CLK_SYS_TIMER0_BITS |
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

    /* enable interrupt for alarm */
    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
    irq_set_exclusive_handler(hardware_alarm_get_irq_num(ALARM_NUM), alarm_irq);
    irq_set_enabled(hardware_alarm_get_irq_num(ALARM_NUM), true);

    /* first tick will be one interval from now */
    timer_hw->alarm[ALARM_NUM] = timer_hw->timerawl + LED_DELAY_US;

    unsigned alarms_acknowledged = 0;
    while (1) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        while (*(volatile unsigned *)&alarms == alarms_acknowledged) yield();
        alarms_acknowledged++;

        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        while (*(volatile unsigned *)&alarms == alarms_acknowledged) yield();
        alarms_acknowledged++;
    }
}
