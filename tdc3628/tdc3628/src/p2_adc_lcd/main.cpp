#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdint.h>

#define USE_INTERRUPT 0        // 0 = polling, 1 = interrupt
#define ADC_CHANNEL   0        // ADC0 = PC0
#define N_SAMPLES     16

// ---------- LCD 1602 4-bit (TODO: sửa theo jumper) ----------
// D4..D7 = PD4..PD7, RS = PB0, E = PB1, RW nối GND
#define LCD_RS PB0
#define LCD_E  PB1

static void lcd_nibble(uint8_t n) {
    PORTD = (PORTD & 0x0F) | (n << 4);
    PORTB |= (1 << LCD_E);
    _delay_us(1);
    PORTB &= ~(1 << LCD_E);
    _delay_us(50);
}
static void lcd_send(uint8_t v, bool rs) {
    if (rs) PORTB |= (1 << LCD_RS); else PORTB &= ~(1 << LCD_RS);
    lcd_nibble(v >> 4);
    lcd_nibble(v & 0x0F);
}
static void lcd_cmd(uint8_t c)  { lcd_send(c, false); if (c <= 2) _delay_ms(2); }
static void lcd_data(uint8_t d) { lcd_send(d, true); }
static void lcd_init() {
    DDRD |= 0xF0;
    DDRB |= (1 << LCD_RS) | (1 << LCD_E);
    _delay_ms(50);
    PORTB &= ~(1 << LCD_RS);
    lcd_nibble(0x03); _delay_ms(5);
    lcd_nibble(0x03); _delay_us(150);
    lcd_nibble(0x03);
    lcd_nibble(0x02);              // chuyển sang 4 bit
    lcd_cmd(0x28);                 // 4 bit, 2 dòng, 5x8
    lcd_cmd(0x0C);                 // bật hiển thị, tắt con trỏ
    lcd_cmd(0x06);
    lcd_cmd(0x01);                 // xóa màn hình
}
static void lcd_goto(uint8_t row, uint8_t col) {
    lcd_cmd(0x80 | ((row ? 0x40 : 0x00) + col));
}
static void lcd_print(const char *s) { while (*s) lcd_data(*s++); }

// ---------- ADC ----------
static void adc_init() {
    ADMUX  = (1 << REFS0) | (ADC_CHANNEL & 0x07);                        // tham chiếu AVcc
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);   // prescaler 128
#if USE_INTERRUPT
    ADCSRA |= (1 << ADIE);
    sei();
#endif
}

#if USE_INTERRUPT
volatile uint16_t adc_raw;
volatile bool     adc_done;
ISR(ADC_vect) {
    adc_raw  = ADC;
    adc_done = true;
}
static uint16_t adc_read() {
    adc_done = false;
    ADCSRA |= (1 << ADSC);
    while (!adc_done);
    return adc_raw;
}
#else
static uint16_t adc_read() {
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}
#endif

static uint16_t adc_read_avg() {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < N_SAMPLES; i++) sum += adc_read();
    return sum / N_SAMPLES;
}

int main() {
    lcd_init();
    adc_init();

    char line[17];
    while (true) {
        uint16_t raw = adc_read_avg();
        uint32_t mv  = (uint32_t)raw * 5000UL / 1023UL;     // mV, Vref = 5 V

        lcd_goto(0, 0);
        snprintf(line, sizeof line, "ADC:%4u", raw);
        lcd_print(line);

        lcd_goto(1, 0);
        snprintf(line, sizeof line, "V:%u.%03u V  ", (unsigned)(mv / 1000), (unsigned)(mv % 1000));
        lcd_print(line);

        _delay_ms(200);
    }
}
