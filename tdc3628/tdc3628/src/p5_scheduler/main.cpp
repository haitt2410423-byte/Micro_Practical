#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdio.h>
#include <stdint.h>
#include "uart.h"

// ---------- Tick 1 ms: Timer2 CTC ----------
volatile uint32_t ms_ticks;
ISR(TIMER2_COMPA_vect) { ms_ticks++; }

static void tick_init() {
    TCCR2A = (1 << WGM21);                 // CTC
    TCCR2B = (1 << CS22);                  // prescaler 64 -> 250 kHz
    OCR2A  = 249;                          // 250 tick = 1 ms
    TIMSK2 = (1 << OCIE2A);
    sei();
}
static uint32_t millis() {
    uint32_t t;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { t = ms_ticks; }
    return t;
}

// ---------- ADC (polling) ----------
static void adc_init() {
    ADMUX  = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}
static uint16_t adc_read(uint8_t ch) {
    ADMUX = (ADMUX & 0xF0) | (ch & 0x0F);
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

// ---------- PWM LED: Timer0, OC0A = PD6 ----------
static void pwm_init() {
    DDRD  |= (1 << PD6);
    TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);
    TCCR0B = (1 << CS01) | (1 << CS00);
    OCR0A  = 0;
}

// ---------- Dữ liệu dùng chung giữa các task ----------
static uint16_t adc_value;     // Task 1
static uint16_t sensor_value;  // Task 3
static uint8_t  duty;          // Task 2

// ---------- Các task ----------
static void task_adc()    { adc_value = adc_read(0); }              // mỗi 10 ms
static void task_pwm()    { duty += 5; OCR0A = duty; }              // mỗi 20 ms (uint8_t tự quay vòng)
static void task_sensor() { sensor_value = adc_read(1); }           // mỗi 100 ms
static void task_uart() {                                           // mỗi 1000 ms
    char buf[48];
    snprintf(buf, sizeof buf, "t=%lu ms ADC=%u SENS=%u DUTY=%u\r\n",
             (unsigned long)millis(), adc_value, sensor_value, duty);
    uart_puts(buf);
}

// ---------- Scheduler ----------
struct Task {
    void (*run)();
    uint16_t period;
    uint32_t last;
};

static Task tasks[] = {
    { task_adc,    10,   0 },
    { task_pwm,    20,   0 },
    { task_sensor, 100,  0 },
    { task_uart,   1000, 0 },
};

int main() {
    tick_init();
    adc_init();
    pwm_init();
    uart_init();

    while (true) {
        uint32_t now = millis();
        for (auto &t : tasks) {
            if (now - t.last >= t.period) {
                t.last += t.period;        // giữ nhịp đều, không bị trôi
                t.run();
            }
        }
    }
}
