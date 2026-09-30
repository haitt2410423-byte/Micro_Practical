#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

// ---------- LED PWM: Timer0, OC0A = PD6 ----------
static void pwm_led_init() {
    DDRD  |= (1 << PD6);
    TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);  // Fast PWM, non-inverting
    TCCR0B = (1 << CS01) | (1 << CS00);                    // prescaler 64 -> ~976 Hz
    OCR0A  = 0;
}
static inline void led_set(uint8_t duty) { OCR0A = duty; }   // 0..255

// ---------- Servo: Timer1, OC1A = PB1 ----------
// 16 MHz / 8 = 2 MHz -> 0,5 us/tick. TOP = 39999 -> 20 ms (50 Hz)
// 1 ms = 2000 tick, 2 ms = 4000 tick
static void servo_init() {
    DDRB  |= (1 << PB1);
    TCCR1A = (1 << COM1A1) | (1 << WGM11);                 // non-inverting, mode 14
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);    // Fast PWM TOP=ICR1, prescaler 8
    ICR1   = 39999;
    OCR1A  = 3000;                                         // giữa (1,5 ms)
}
static void servo_angle(uint8_t deg) {                     // 0..180
    if (deg > 180) deg = 180;
    OCR1A = 2000 + (uint32_t)deg * 2000UL / 180UL;
}

int main() {
    pwm_led_init();
    servo_init();

    while (true) {
        for (uint16_t i = 0; i <= 180; i++) {              // LED sáng dần, servo 0 -> 180
            servo_angle(i);
            led_set((uint32_t)i * 255UL / 180UL);
            _delay_ms(20);
        }
        for (int16_t i = 180; i >= 0; i--) {               // LED tối dần, servo 180 -> 0
            servo_angle(i);
            led_set((uint32_t)i * 255UL / 180UL);
            _delay_ms(20);
        }
    }
}
