#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

// Driver STEP/DIR (A4988 / DRV8825). Nối jumper:
// PB1 -> Pul, PB2 -> Dir, PB3 -> En (sửa nếu cắm chân khác)
#define PUL_PIN PB1
#define DIR_PIN PB2
#define EN_PIN  PB3

#define FULL_STEPS_REV 200UL   // động cơ 1,8 độ/bước (kiểm tra nhãn động cơ)
#define MICROSTEP      1UL     // theo cấu hình vi bước trên driver

static void delay_us_var(uint16_t us) {       // _delay_us cần hằng số nên tự lặp
    while (us >= 10) { _delay_us(10); us -= 10; }
}

static void stepper_init() {
    DDRB  |= (1 << PUL_PIN) | (1 << DIR_PIN) | (1 << EN_PIN);
    PORTB &= ~((1 << PUL_PIN) | (1 << DIR_PIN));
    PORTB |= (1 << EN_PIN);                   // EN mức cao = tắt driver (active-low)
}
static void stepper_enable(bool on) {
    if (on) PORTB &= ~(1 << EN_PIN); else PORTB |= (1 << EN_PIN);
}

static void stepper_move(uint32_t steps, bool cw, uint16_t period_us) {
    if (cw) PORTB |= (1 << DIR_PIN); else PORTB &= ~(1 << DIR_PIN);
    _delay_us(5);
    stepper_enable(true);
    while (steps--) {
        PORTB |= (1 << PUL_PIN);
        _delay_us(5);
        PORTB &= ~(1 << PUL_PIN);
        delay_us_var(period_us);
    }
    stepper_enable(false);
}

static void stepper_rotate(uint16_t deg, bool cw, uint16_t period_us) {
    stepper_move((uint32_t)deg * FULL_STEPS_REV * MICROSTEP / 360UL, cw, period_us);
}

int main() {
    stepper_init();
    while (true) {
        stepper_rotate(360, true,  2000);     // 1 vòng thuận
        _delay_ms(1000);
        stepper_rotate(360, false, 2000);     // 1 vòng nghịch
        _delay_ms(1000);
        stepper_rotate(90,  true,  4000);     // 90 độ, chậm hơn
        _delay_ms(1000);
    }
}
