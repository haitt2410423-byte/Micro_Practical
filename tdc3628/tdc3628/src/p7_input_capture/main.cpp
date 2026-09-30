#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdint.h>
#include "uart.h"

#define T1_HZ        (F_CPU / 8UL)          // prescaler 8 -> 2 MHz (0,5 us/tick)
#define TICKS_PER_US (T1_HZ / 1000000UL)    // = 2

volatile uint16_t last_rise, period_ticks, high_ticks;
volatile bool synced, new_data;

ISR(TIMER1_CAPT_vect) {
    uint16_t t = ICR1;
    if (TCCR1B & (1 << ICES1)) {            // vừa bắt cạnh LÊN
        if (synced) {
            period_ticks = t - last_rise;
            new_data = true;
        }
        last_rise = t;
        synced = true;
        TCCR1B &= ~(1 << ICES1);            // lần sau bắt cạnh XUỐNG
    } else {                                // vừa bắt cạnh XUỐNG
        high_ticks = t - last_rise;
        TCCR1B |= (1 << ICES1);             // lần sau bắt cạnh LÊN
    }
    TIFR1 |= (1 << ICF1);                   // xóa cờ sau khi đổi cạnh
}

static void gen_init() {                    // nguồn tín hiệu thử ~976 Hz tại PD6
    DDRD  |= (1 << PD6);
    TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);
    TCCR0B = (1 << CS01) | (1 << CS00);
    OCR0A  = 77;
}

static void capture_init() {
    DDRB  &= ~(1 << PB0);                   // ICP1 = PB0, input
    TCCR1A = 0;                             // Normal mode
    TCCR1B = (1 << ICNC1) | (1 << ICES1) | (1 << CS11);   // lọc nhiễu, cạnh lên trước, prescaler 8
    TIFR1  = (1 << ICF1);
    TIMSK1 = (1 << ICIE1);
    sei();
}

int main() {
    uart_init();
    gen_init();
    capture_init();

    static const uint8_t duties[] = {26, 77, 128, 179, 230};
    uint8_t idx = 0;
    char buf[64];

    while (true) {
        if (new_data) {
            uint16_t p, h;
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                p = period_ticks;
                h = high_ticks;
                new_data = false;
            }
            uint32_t f  = T1_HZ / p;
            uint32_t pu = p / TICKS_PER_US;
            uint32_t hu = h / TICKS_PER_US;
            uint32_t d  = (uint32_t)h * 100UL / p;
            snprintf(buf, sizeof buf, "f=%lu Hz  T=%lu us  high=%lu us  duty=%lu%%\r\n",
                     (unsigned long)f, (unsigned long)pu, (unsigned long)hu, (unsigned long)d);
            uart_puts(buf);

            _delay_ms(500);
            OCR0A = duties[idx];
            idx = (idx + 1) % 5;
        }
    }
}
