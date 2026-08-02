# Desk Climate Monitor

A desk monitor that displays temperature and humidity built on an Arduino Nano. Reads an SHT31-DIS sensor over I2C and shows live readings on a 128x64 OLED. Written in Arduino C++.

**Status:** COMPLETE

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
 - Adafruit SHT31 library (with Adafruit BusIO, Adafruit Unified Sensor)

## Wiring

Both devices share the I2C bus. Everything runs at 3.3V, distributed via the
breadboard power rails.

### SSD1306 OLED (4-pin)

| Device pin | Nano pin |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | A4 |
| SCL | A5 |

### SHT31-DIS module (2x3 header)
 
| Pin | Name | Nano pin |
|---|---|---|
| 1 | NRESET | 3V3 |
| 2 | SDA | A4 |
| 3 | SCL | A5 |
| 4 | NC | — |
| 5 | GND | GND |
| 6 | VCC | 3V3 |

Three things about this module are not obvious and are not printed on the board.
 
The pin numbering zigzags across the two columns rather than running along the
board edge — 1, 2, 3 down one column and 4, 5, 6 down the other. Check each pin
against the manufacturer's diagram rather than counting along the header.
 
The 2x3 header cannot be plugged into a breadboard directly. The two columns sit
0.1" apart, so both land in the same breadboard row, which would short pin 2 (SDA)
to pin 5 (GND) and pin 3 (SCL) to pin 6 (VCC). No orientation avoids this; the
0.3" centre channel is too wide to straddle with 0.1" pins. Connect with
female-to-male jumpers so the module never touches the breadboard.
 
ADDR is not broken out on this module (confirmed with the seller), so the address
is fixed at 0x44 and cannot be changed. Two of these sensors therefore cannot share
one hardware I2C bus. The planned two-sensor cross-validation will need a software
I2C bus on two spare digital pins, or power gating so only one sensor is awake at a
time.
 
NRESET is tied to VCC rather than left unconnected. The pin is internally pulled up
and active low, so floating is nominally safe, but the datasheet recommends tying it
to VDD when unused (§3.6) and an unconnected breadboard wire is a good antenna for
spurious resets.

### Power and levels

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
`delay()` halts the entire chip, making concurrent operations impossible. Each timed
task carries its own last-fired timestamp and interval constant, and the tasks do not
reference each other — nesting one inside another's `if` block couples their periods
and produces timing that only works when the intervals happen to divide evenly.
Adding a task requires only another timestamp, no restructuring. This matters because
the planned follow-up build needs simultaneous shot timing, load cell reads, and
display updates.
 
**Sensor reads block.** `Adafruit_SHT31::readTemperature()` blocks for the full
measurement duration — up to 15 ms at high repeatability. This is acceptable at a
1 Hz read rate but incompatible with the follow-up build. The alternatives are the
sensor's periodic acquisition mode (datasheet §4.5), where the sensor measures on its
own schedule and the master fetches the last result, or raw `Wire` transactions with
a self-managed wait between command and read.
 
**Failed reads return NaN.** The library signals a failed measurement by returning
NaN rather than by an error code, and NaN propagates silently through arithmetic.
Readings are initialised to `NAN` at startup and guarded with `isnan()` before use,
so a value is never displayed unless it came from a successful measurement.

**Float to text.** `u8g2.drawStr()` takes a `const char*`, so readings are converted
with `dtostrf(value, width, precision, buffer)` before they can be drawn. `dtostrf`
writes into a caller-supplied `char` array with no bounds checking, so the buffer must
be sized for the worst case — sign, digits, decimal point, decimal places, and the
terminating null — or it writes past the end into adjacent memory.

Temperature and humidity get separate buffers rather than one reused. With a single
buffer, both conversions would run before either value is drawn, and the second would
overwrite the first — the temperature line would display the humidity reading. The
alternative, interleaving conversion and drawing, would put `dtostrf` calls inside the
page loop, where side effects don't belong.

The buffers are global for the same reason the readings are: the sensor block writes
them and the display block reads them, on independent schedules. A buffer scoped
inside `loop()` would be destroyed between the two.
 
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

**Readings look plausible but may be stale.** A stuck read and a working sensor both
print floats. Breathe gently on the sensor from a few centimetres away — humidity
should rise within a second or two and decay back over 10–20 seconds. Response to a
change is the proof, not the value itself.

## Milestones

- [x] 1. Toolchain, non-blocking blink
- [x] 2. OLED "Hello World!"
- [x] 3. Read SHT31 over serial
- [x] 4. Live readings on display

## Repository layout

    firmware/
      blink_baseline/       unmodified Arduino Blink, compile baseline
      blink_millis/         non-blocking blink using millis()
      oled_helloworld/      OLED text output, two independent millis() timers
      sht31_serial/         SHT31 readings to serial, 1 Hz, NaN-guarded
      desk-climate-monitor/ combined sensor and display, two independent timers
