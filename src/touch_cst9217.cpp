// Přepsáno pro CST820 kapacitní dotyk (I2C) - Waveshare ESP32-S3-Touch-AMOLED-1.43C
#include "touch_cst9217.h" // Ponecháno původní jméno headeru, aby se nemusela měnit struktura
#include "config.h"
#include <Arduino.h>
#include <Wire.h>

#define CST820_REG_STATUS 0x02  // Registr obsahující počet stisknutých bodů
#define CST820_REG_DATA   0x03  // Registr, kde začínají X/Y souřadnice prvního bodu

// Čtení z 8bitového registru čipu CST820
static bool cst_read_reg(uint8_t reg, uint8_t *data, uint8_t len) {
    Wire.beginTransmission((uint8_t)I2C_ADDR_TOUCH);
    Wire.write(reg);
    if (Wire.endTransmission(true) != 0) return false;
    
    if (Wire.requestFrom((uint8_t)I2C_ADDR_TOUCH, len) < len) return false;
    for (uint8_t i = 0; i < len; ++i) {
        data[i] = Wire.read();
    }
    return true;
}

bool touch_begin() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);

    // Hardwarový reset dotykového čipu
    pinMode(PIN_TP_RST, OUTPUT);
    digitalWrite(PIN_TP_RST, LOW);
    delay(10);
    digitalWrite(PIN_TP_RST, HIGH);
    delay(50);
    pinMode(PIN_TP_INT, INPUT);

    // Rychlý test komunikace (přečteme stavový registr)
    uint8_t status = 0;
    if (cst_read_reg(CST820_REG_STATUS, &status, 1)) {
        Serial.println("[touch] CST820 na adrese 0x15 reaguje správně.");
    } else {
        Serial.println("[touch] CST820 neodpovídá! Zkontroluj I2C piny a adresu.");
    }
    return true;
}

bool touch_read(uint16_t *ox, uint16_t *oy) {
    uint8_t points = 0;
    
    // Nejprve přečteme stav, jestli na displeji vůbec je nějaký prst
    if (!cst_read_reg(CST820_REG_STATUS, &points, 1)) return false;
    if (points == 0 || points > 5) return false; // Žádný dotyk nebo neplatná data

    // Přečteme 4 bajty dat pro první dotykový bod (X_high, X_low, Y_high, Y_low)
    uint8_t d[4] = {0};
    if (!cst_read_reg(CST820_REG_DATA, d, 4)) return false;

    // Dekódování souřadnic pro CST820 (12-bitové hodnoty rozdělené do bajtů)
    uint16_t x = ((uint16_t)(d[0] & 0x0F) << 8) | d[1];
    uint16_t y = ((uint16_t)(d[2] & 0x0F) << 8) | d[3];

    // Ošetření přetečení rozlišení displeje
    if (x > SCREEN_W - 1) x = SCREEN_W - 1;
    if (y > SCREEN_H - 1) y = SCREEN_H - 1;
    
    // Zrcadlení souřadnic (pokud je v config.h nastaveno na true)
    if (TP_MIRROR_X) x = (SCREEN_W - 1) - x;
    if (TP_MIRROR_Y) y = (SCREEN_H - 1) - y;

    *ox = x;
    *oy = y;
    return true;
}
