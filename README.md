# Desk Climate Monitor

A desk monitor that displays temperature and humidity built on an Arduino Nano. Reads an SHT31-DIS sensor over I2C and shows live readings on a 128x64 OLED. Written in Arduino C++.

**Status:** In progress - milestone 2 of 4 complete.

## Hardware

| Component | Part | Note |
|---|---|---|
| Microcontroller | Arduino Nano V3.0 (ATmega328P, CH340) | 5V, 2KB SRAM |
| Sensor | SHT31-DIS | I2C 0x44, +/-0.2C, +/-2%RH, 2.4-5.5V |
| Display | SSD1306 0.96" 128x64 | I2C 0x3C, white, no onboard regulator |

## Dependencies

 - Arduino IDE 2.x
 - CH340 USB-serial driver
 - U8g2 by oliver, v2.36.19

## Wiring

Both devices share the I2C bus. Everything runs at 3.3V.

| Device pin | Nano pin |
|---|---|
| VIN / VCC | 3V3 |
| GND | GND |
| SDA | A4 |
| SCL | A5 |

The OLED module has no onboard voltage regulator — inspection of the back of the
board found only two-terminal passives, no SOT-23 package near VCC. VCC therefore
connects to the SSD1306's logic supply directly, which is rated 1.65–3.3V with an
absolute maximum of 4.0V. Powering it from the Nano's 5V pin risks gradual damage
that may not show up immediately.

The SHT31 runs on the same 3.3V rail rather than 5V, even though it tolerates 5V.
Each module carries its own I2C pull-up resistors tied to its own VCC. Splitting
the devices across two rails would let the 5V device's pull-ups drag the shared bus
to 5V, which the SSD1306's pins would then see.
 
This is also why a 5V microcontroller can share a 3.3V bus without a level shifter.
I2C is open-drain: no device ever drives SDA or SCL high. Devices only pull the
lines *low*, and the pull-up resistors return them high. The bus's high level is
therefore set entirely by whatever the pull-ups connect to — 3.3V here — regardless
of the ATmega running at 5V.
 
One caveat. The ATmega328P at 5V needs roughly 3.0V on an input pin to register a
logic high. 3.3V clears that, but not by a wide margin. If the bus turns flaky after
adding hardware, check this before suspecting the firmware.
 
Note that the Nano's 3.3V rail on CH340 clones is derived from the USB-serial chip
rather than a dedicated LDO, and its current capability is limited. This is fine for
a page-buffered text display and one sensor; adding load may require an external
regulator.

## Design notes

**Sensor selection.** The SHT4x family was ruled out — it is rated 1.08–3.6V and
would need level shifting to coexist with 5V logic. The SHT3x family runs 2.4–5.5V,
which left the choice of supply rail open rather than forcing it.
 
**Non-blocking timing.** All timing uses `millis()` rather than `delay()`.
`delay()` halts the entire chip, making concurrent operations impossible. The
current firmware runs two independent timed tasks (display refresh and status LED),
each with its own last-fired timestamp and interval. Adding a third task requires
only another timestamp — no restructuring. This matters because the planned
follow-up build needs simultaneous shot timing, load cell reads, and display
updates.

**SRAM budget.** The ATmega328P has 2048 bytes of SRAM. A full SSD1306 framebuffer
is 1024 bytes — half the total. This firmware uses the `_1_` page-buffer constructor
(`U8G2_SSD1306_128X64_NONAME_1_HW_I2C`), which holds one 128-byte page at a time
instead.
 
The tradeoff is the rendering API. Page-buffer mode requires the
`firstPage()`/`nextPage()` loop, whose body executes eight times per frame — once
per page — with U8g2 keeping only the slice belonging to the current page. Every
drawing call must therefore be repeated on every pass, and the loop body must
contain no side effects. Sensor reads, timestamp updates, and counter increments
happen before `firstPage()`; their results are stored in variables that the loop
body only reads.

## Troubleshooting

**First check when hardware misbehaves:** run `File > Examples > Wire > i2c_scanner`
at 9600 baud. Expected addresses are 0x3C (OLED) and 0x44 (SHT31). A device that
acknowledges its address confirms power, both signal lines, and that the part is
alive — four things from one result. If the scanner finds the device, the problem
is software.
 
**Upload fails with `stk500_recv(): not in sync`.** This CH340 clone requires
`Tools > Processor > ATmega328P (Old Bootloader)`.
 
**Upload fails with `Access is denied` on the COM port.** Something else holds the
port open — usually the Serial Monitor. Close it, then upload. If that fails,
unplug and replug the Nano, then restart the IDE.
 
**Display blank after a successful upload.** Check that `setFont()` is called before
any text drawing; with no font set, U8g2 draws nothing and reports no error. Also
check the y-coordinate: `drawStr()` positions text by its baseline, not its top
edge, so y=0 draws entirely above the visible area.

## Milestones

- [x] 1. Toolchain, non-blocking blink
- [x] 2. OLED "Hello World!"
- [ ] 3. Read SHT31 over serial
- [ ] 4. Live readings on display

## Repository layout

    firmware/
      blink_baseline/    unmodified Arduino Blink, compile baseline
      blink_millis/      non-blocking blink using millis()
      oled_helloworld/   OLED text output, two independent millis() timers
