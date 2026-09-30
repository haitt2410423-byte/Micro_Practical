#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "uart.h"

#define I2C_SCL_HZ 100000UL
#define AHT20_ADDR 0x38

// Giao dịch I2C: START -> [địa chỉ 7 bit + bit R/W] -> ACK -> DATA (+ACK) ... -> STOP

static bool twi_wait() {                     // chờ TWINT, có timeout
    uint16_t t = 60000;
    while (!(TWCR & (1 << TWINT))) if (--t == 0) return false;
    return true;
}

static void i2c_init() {
    TWSR = 0;                                // prescaler 1
    TWBR = (F_CPU / I2C_SCL_HZ - 16) / 2;    // = 72 -> SCL 100 kHz
    TWCR = (1 << TWEN);
}

static bool i2c_start() {
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    if (!twi_wait()) return false;
    uint8_t s = TWSR & 0xF8;
    return s == 0x08 || s == 0x10;           // START / repeated START
}

static bool i2c_write(uint8_t data) {        // dùng cho cả byte địa chỉ và dữ liệu
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    if (!twi_wait()) return false;
    uint8_t s = TWSR & 0xF8;
    return s == 0x18 || s == 0x28 || s == 0x40;   // SLA+W ACK / DATA ACK / SLA+R ACK
}

static uint8_t i2c_read_ack() {              // đọc 1 byte, gửi ACK (còn đọc tiếp)
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
    twi_wait();
    return TWDR;
}

static uint8_t i2c_read_nack() {             // đọc byte cuối, gửi NACK
    TWCR = (1 << TWINT) | (1 << TWEN);
    twi_wait();
    return TWDR;
}

static void i2c_stop() {
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    uint16_t t = 60000;
    while ((TWCR & (1 << TWSTO)) && --t);
}

// ---------- Quét bus ----------
static void i2c_scan() {
    char buf[24];
    uart_puts("Scan I2C...\r\n");
    for (uint8_t a = 0x08; a < 0x78; a++) {
        bool ack = i2c_start() && i2c_write(a << 1);   // SLA+W
        i2c_stop();
        if (ack) {
            snprintf(buf, sizeof buf, "  found 0x%02X\r\n", a);
            uart_puts(buf);
        }
    }
}

// ---------- AHT20 ----------
static bool aht20_init() {
    _delay_ms(40);
    bool ok = i2c_start() && i2c_write(AHT20_ADDR << 1)
           && i2c_write(0xBE) && i2c_write(0x08) && i2c_write(0x00);
    i2c_stop();
    _delay_ms(10);
    return ok;
}

// t10 = nhiệt độ x10 (°C), rh10 = độ ẩm x10 (%)
static bool aht20_read(int16_t &t10, uint16_t &rh10) {
    bool ok = i2c_start() && i2c_write(AHT20_ADDR << 1)
           && i2c_write(0xAC) && i2c_write(0x33) && i2c_write(0x00);
    i2c_stop();
    if (!ok) return false;
    _delay_ms(80);

    uint8_t d[7];
    if (!i2c_start() || !i2c_write((AHT20_ADDR << 1) | 1)) {   // SLA+R
        i2c_stop();
        return false;
    }
    for (uint8_t i = 0; i < 6; i++) d[i] = i2c_read_ack();
    d[6] = i2c_read_nack();                  // byte cuối (CRC) -> NACK
    i2c_stop();

    if (d[0] & 0x80) return false;           // còn bận

    uint32_t rawH = ((uint32_t)d[1] << 12) | ((uint32_t)d[2] << 4) | (d[3] >> 4);
    uint32_t rawT = ((uint32_t)(d[3] & 0x0F) << 16) | ((uint32_t)d[4] << 8) | d[5];
    rh10 = (rawH * 1000UL) >> 20;
    t10  = (int16_t)((int32_t)((rawT * 2000UL) >> 20) - 500);
    return true;
}

int main() {
    uart_init();
    i2c_init();
    PORTC |= (1 << PC4) | (1 << PC5);        // pull-up nội (dự phòng)

    i2c_scan();
    if (!aht20_init()) uart_puts("ERR: AHT20 init failed\r\n");

    char buf[48];
    while (true) {
        int16_t t10; uint16_t rh10;
        if (aht20_read(t10, rh10)) {
            int16_t a = abs(t10);
            snprintf(buf, sizeof buf, "T=%c%d.%d C  RH=%u.%u %%\r\n",
                     t10 < 0 ? '-' : ' ', a / 10, a % 10, rh10 / 10, rh10 % 10);
        } else {
            snprintf(buf, sizeof buf, "ERR: sensor not responding\r\n");
        }
        uart_puts(buf);
        _delay_ms(1000);
    }
}
