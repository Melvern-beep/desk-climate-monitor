/*************************************************** 
  This is an example for the SHT31-D Humidity & Temp Sensor

  Designed specifically to work with the SHT31-D sensor from Adafruit
  ----> https://www.adafruit.com/products/2857

  These sensors use I2C to communicate, 2 pins are required to  
  interface
 ****************************************************/
 
#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"


const unsigned long UPDATE_INTERVAL_MS = 1000;

unsigned long lastUpdateMs=0;
float t = NAN;
float h = NAN;

Adafruit_SHT31 sht31 = Adafruit_SHT31();

void setup() {
  Serial.begin(9600);

  Serial.println("SHT31 test");
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }
}

void loop() {
  unsigned long nowMs = millis();

  if (nowMs - lastUpdateMs >= UPDATE_INTERVAL_MS){
    lastUpdateMs = nowMs;
    t = sht31.readTemperature();
    h = sht31.readHumidity();
    
    if (! isnan(t)) {  // check if 'is not a number'
      Serial.print("Temp *C = "); Serial.print(t); Serial.print("\t\t");
    } else { 
      Serial.println("Failed to read temperature");
    }
    
    if (! isnan(h)) {  // check if 'is not a number'
      Serial.print("Hum. % = "); Serial.println(h);
    } else { 
      Serial.println("Failed to read humidity");
    }
  }
}

