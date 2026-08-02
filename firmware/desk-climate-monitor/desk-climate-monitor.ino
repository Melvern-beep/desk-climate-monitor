#include "Adafruit_SHT31.h"
#include <Arduino.h>
#include <U8g2lib.h>
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

const unsigned long UPDATE_SENSOR_INTERVAL_MS = 300;
const unsigned long UPDATE_OLED_INTERVAL_MS = 500;


unsigned long lastSensorUpdateMs = 0;
unsigned long lastOledUpdateMs = 0;
float t = NAN;
float h = NAN;
char tBuf[8] = "";
char hBuf[8] = "";

Adafruit_SHT31 sht31 = Adafruit_SHT31();

void setup() {
  Serial.begin(9600);
  u8g2.begin();

  Serial.println("SHT31 test");
  if (!sht31.begin(0x44)) {  // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

  u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
}

void loop() {
  unsigned long nowMs = millis();

  if (nowMs - lastSensorUpdateMs >= UPDATE_SENSOR_INTERVAL_MS) {
    lastSensorUpdateMs = nowMs;
    t = sht31.readTemperature();
    h = sht31.readHumidity();
    dtostrf(t, 5, 1, tBuf);
    dtostrf(h, 5, 1, hBuf);
  }

  if (nowMs - lastOledUpdateMs >= UPDATE_OLED_INTERVAL_MS) {
    lastOledUpdateMs = nowMs;
    u8g2.firstPage();
    do {
      if (!isnan(t)) {
        u8g2.drawStr(0, 10, "Temp *C = ");
        u8g2.drawStr(60, 10, tBuf);
      } else {
        u8g2.drawStr(0, 10, "Failed to read temperature");
      }
      if (!isnan(h)) {
        u8g2.drawStr(0, 30, "Hum. % = ");
        u8g2.drawStr(60, 30, hBuf);
      } else {
        u8g2.drawStr(0, 30, "Failed to read humidity");
      }
    } while (u8g2.nextPage());
  }
}
