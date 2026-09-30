#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

// ---------- Khai báo chân (TODO: khớp với jumper trên board) ----------
// Nút B1..B4 -> PD2..PD5
#define BTN_PORT  PORTD
#define BTN_DDR   DDRD
#define BTN_PINR  PIND
#define BTN1 PD2
#define BTN2 PD3
#define BTN3 PD4
#define BTN4 PD5
#define BTN_ACTIVE_LOW 1      // 1: nhấn = mức thấp, 0: nhấn = mức cao

// LED L1 -> PC0
#define LED_PORT  PORTC
#define LED_DDR   DDRC
#define LED1 PC0

// Relay 1, 2 và buzzer -> PB0, PB1, PB2
#define OUT_PORT  PORTB
#define OUT_DDR   DDRB
#define RELAY1 PB0
#define RELAY2 PB1
#define BUZZER PB2
#define RELAY_ACTIVE_LOW  0   // 1 nếu module relay kích mức thấp
#define BUZZER_ACTIVE_LOW 0

static inline void out_set(volatile uint8_t &port, uint8_t pin, bool on, bool active_low) {
    if (on ^ active_low) port |= (1 << pin); else port &= ~(1 << pin);
}
static inline bool pressed(uint8_t pin) {
    bool level = BTN_PINR & (1 << pin);
    return BTN_ACTIVE_LOW ? !level : level;
}
// Trả về true đúng một lần mỗi lần nhấn (chống dội, chờ nhả nút)
static bool button_event(uint8_t pin) {
    if (!pressed(pin)) return false;
    _delay_ms(20);
    if (!pressed(pin)) return false;
    while (pressed(pin));
    _delay_ms(20);
    return true;
}

int main() {
    BTN_DDR  &= ~((1 << BTN1) | (1 << BTN2) | (1 << BTN3) | (1 << BTN4));
    BTN_PORT |=  ((1 << BTN1) | (1 << BTN2) | (1 << BTN3) | (1 << BTN4));   // pull-up nội

    LED_DDR |= (1 << LED1);
    OUT_DDR |= (1 << RELAY1) | (1 << RELAY2) | (1 << BUZZER);
    out_set(LED_PORT, LED1,   false, false);
    out_set(OUT_PORT, RELAY1, false, RELAY_ACTIVE_LOW);
    out_set(OUT_PORT, RELAY2, false, RELAY_ACTIVE_LOW);
    out_set(OUT_PORT, BUZZER, false, BUZZER_ACTIVE_LOW);

    bool led = false, relay1 = false, relay2 = false;

    while (true) {
        if (button_event(BTN1)) {            // B1: đảo LED
            led = !led;
            out_set(LED_PORT, LED1, led, false);
        }
        if (button_event(BTN2)) {            // B2: đảo relay 1
            relay1 = !relay1;
            out_set(OUT_PORT, RELAY1, relay1, RELAY_ACTIVE_LOW);
        }
        if (button_event(BTN3)) {            // B3: đảo relay 2
            relay2 = !relay2;
            out_set(OUT_PORT, RELAY2, relay2, RELAY_ACTIVE_LOW);
        }
        // B4: giữ nút thì buzzer kêu
        out_set(OUT_PORT, BUZZER, pressed(BTN4), BUZZER_ACTIVE_LOW);
    }
}
