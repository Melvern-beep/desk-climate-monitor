# Desk Climate Monitor



A desk monitor that displays temperature and humidity built on an Arduino Nano.

Reads an SHT31-DIS sensor over I2C and shows live readings on a 128x64 OLED.

Written in Arduino C++.



**Status:** In progress - milestone 1 of 4 complete.



## Hardware



| Component | Part | Note |

|---|---|---|

| Microcontroller | Arduino Nano V3.0 (ATmega328P, CH340) | 5V, 2KB SRAM |

| Sensor | SHT31-DIS | I2C 0x44, +/-0.2C, +/-2%RH, 2.4-5.5V |

| Display | SSD1306 0.96" 128x64 | I2C 0x3C, white |



## Wiring



Both devices share the I2C bus.



| Device pin | Nano pin |

|---|---|

| VIN / VCC | 5V |

| GND | GND |

| SDA | A4 |

| SCL | A5 |



## Design notes



**Sensor selection.** Originally planned around a DHT22. Switched to

SHT31-DIS after finding the SHT4x family is rated 1.08-3.6V, which would

need level shifting on a 5V Nano. The SHT3x family runs 2.4-5.5V and

connects directly.



**Non-blocking timing.** All timing uses millis() rather than delay().

delay() halts the entire chip, which makes concurrent operations

impossible. This project only needs one timed task, but the pattern is

required for the follow-up build.



**SRAM budget.** The ATmega328P has 2048 bytes of SRAM. A full SSD1306

framebuffer is 1024 bytes - half the total.



## Milestones



- [x] 1. Toolchain, non-blocking blink

- [ ] 2. OLED "Hello World"

- [ ] 3. Read SHT31 over serial

- [ ] 4. Live readings on display



## Repository layout



&#x20;   firmware/

&#x20;     blink_baseline/    unmodified Arduino Blink, compile baseline

&#x20;     blink_millis/      non-blocking blink using millis()

