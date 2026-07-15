
# ESP32 Seven-Segment Binary Counter

A small ESP32 project written in C with ESP-IDF. The counter displays the current value in decimal on a two-digit seven-segment display while six LEDs show the same value in binary.

<p align="center">
<img width="300" alt="IMG_9510" src="https://github.com/user-attachments/assets/f7cb0edd-92b1-4d4b-838b-92a945998d18" />
<img width="300" alt="IMG_9509" src="https://github.com/user-attachments/assets/9357cf2a-9914-4681-a36c-90b42907762f" />
<img width="300" alt="IMG_9508" src="https://github.com/user-attachments/assets/10c6d870-b4f2-4d33-b2ca-2af1188d3f09" />
</p>

## Features

- Counts from 0 to 63
- Displays the decimal value on a two-digit seven-segment display
- Displays the same value as a six-bit binary number using LEDs
- Pushbutton-controlled count increment
- Wraps from 63 back to 0
- Seven-segment digit multiplexing
- Button debouncing
- Implemented in C using ESP-IDF

## Example

When the decimal display shows 26, the binary LEDs represent:


26 decimal = 011010 binary


When the display shows 63, all six binary LEDs are illuminated:


63 decimal = 111111 binary


## Hardware

- ESP32 ESP-WROOM-32 development board
- Two-digit seven-segment display
- Six green LEDs
- Momentary pushbutton
- Current-limiting resistors
- Breadboard and jumper wires
- USB power/programming cable

## Operation

Each valid button press increments the counter by one. The firmware then:

1. Updates the six binary LED outputs.
2. Converts the value into tens and ones digits.
3. Refreshes the multiplexed seven-segment display.
4. Returns the counter to zero after reaching 63.

### Prerequisites

- ESP-IDF 5.5
- ESP32 toolchain
- USB connection to the ESP32

### Build

powershell
idf.py set-target esp32
idf.py build

## Possible Improvements

- Add automatic counting with an adjustable interval
- Add count-up and count-down controls
- Store the count in nonvolatile memory
- Replace delay-based button handling with GPIO interrupts
- Separate the seven-segment driver from the application logic
