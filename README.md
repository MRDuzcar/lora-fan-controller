# LoRa Fan Controller

An ESP32-based wireless controller for four-pin PC fans. A handheld transmitter uses a potentiometer or buttons to select a speed setting and sends commands over 915 MHz LoRa. The receiver generates 25 kHz fan PWM through a 2N2222 interface and controls a MOSFET module for independent power switching.

The project developed from a local fan-control prototype into a wireless system with custom transmitter and receiver PCBs designed in EasyEDA, including custom component footprints.

## Project status

The wireless prototype has been tested, and the fabricated PCBs have been received. Soldering and testing of the assembled boards are pending. The receiver PCB includes connections for six fans; testing of the six-fan assembly is pending.

## Features

- Potentiometer and button control modes with separate mode indicator LEDs.
- Nominal speed settings of 500–2700 RPM in 100 RPM steps, plus a zero setting.
- OLED display showing control mode, master enable, and target setting.
- Independent master enable button and MOSFET fan power switching.
- Fan power disabled when the target is zero or master enable is off.

The displayed RPM is a nominal command mapped to PWM duty, rather than measured feedback. The current firmware does not regulate actual RPM.

## Current firmware

Use these sketches together:

- [Transmitter](firmware/transmitter/transmitter.ino)
- [Receiver](firmware/receiver/receiver.ino)

## Hardware

- Two LILYGO ESP32 LoRa boards.
- Four-pin 12 V PC fan prototype.
- Potentiometer and four pushbuttons.
- Mode indicator LEDs with series resistors.
- SSD1306 128 × 64 I2C OLED.
- 2N2222 PWM interface and MOSFET power-switch module.
- 12 V DC power supply and buck converter for receiver power.

The potentiometer uses a 3.3 V reference. Buttons connect their GPIO inputs to ground when pressed. Power-terminal wiring depends on the MOSFET module and should follow the circuit schematic.

## Pin assignments

### Transmitter

| Function | GPIO |
|---|---:|
| Mode button | 14 |
| Down button | 12 |
| Up button | 13 |
| Master enable button | 0 |
| Potentiometer ADC | 36 |
| Potentiometer mode LED | 25 |
| Button mode LED | 4 |
| OLED SDA | 21 |
| OLED SCL | 22 |

Buttons use `INPUT_PULLUP`, so a pressed button reads LOW. The OLED uses I2C address `0x3C`. A separate supply indicator LED is wired directly to the supply. Leave the GPIO0 button released during startup and uploading.

### Receiver

| Function | GPIO |
|---|---:|
| PWM output to 2N2222 interface | 12 |
| MOSFET control | 13 |
| Master enable indicator LED | 25 |

The 2N2222 stage inverts the PWM signal, which is accounted for in firmware. MOSFET control is active HIGH. The receiver LED indicates master enable; it can remain on while a zero target keeps the fan off.

### LoRa radio on both boards

| Signal | GPIO |
|---|---:|
| SCK | 5 |
| MISO | 19 |
| MOSI | 27 |
| CS | 18 |
| Reset | 23 |
| DIO0 | 26 |

Both boards use a radio frequency of 915 MHz.

## Build and upload

1. Install the appropriate ESP32 board package in Arduino IDE. The firmware requires support for the pin-based `ledcAttach()` and `ledcWrite()` APIs.
2. Install **LoRa by Sandeep Mistry**, **Adafruit GFX Library**, and **Adafruit SSD1306**, including their dependencies.
3. Open each current firmware sketch in its matching folder.
4. Select the board and port for each device, then compile and upload the transmitter and receiver separately.
5. Open Serial Monitor at **115200 baud** to view initialization and command messages.

## Operation

- **MODE:** Switch between potentiometer and button control.
- **UP:** Select 500 from zero, then increase the setting by 100 up to 2700.
- **DOWN:** Reduce the setting by 100; pressing DOWN at 500 selects zero.
- **POWER:** Toggle master enable independently of the target setting.

## Enclosure Design

I designed a custom 3D-printable enclosure to house the project electronics.
The [Enclosure STL](mechanical/Fan%20case.stl) is available for printing.
Printing and fit testing are pending.

Master enable starts off. Selecting zero disables fan power but leaves master enable unchanged. Raising the target can therefore restart the fan if master enable is still on.

## Design changes and development history

Early prototypes measured tachometer pulses and sent actual RPM back to the transmitter. During power-switching tests, tachometer wiring caused backfeeding that prevented the fan from fully turning off. The current design omits tachometer feedback and uses PWM-only speed control.

- [Earlier feedback version](archive/feedback_version/): Preserves the transmitter and receiver with tachometer feedback. It uses a different power-command protocol and must be used as a matched pair.
- [Development examples](examples/): Local fan control, LoRa PING/ACK communication, button and LED tests, and earlier remote-control and OLED experiments. These are standalone bench sketches with version-specific wiring.

## Current limitations

- Actual fan speed is not measured or regulated in the current firmware.
- Commands have no acknowledgement, automatic retry, or link-loss shutdown. A missed packet can leave the receiver in its previous state.
- The OLED shows the transmitter's commanded state, without confirmation from the receiver.

## AI Assistance

The Arduino firmware and project documentation were created with AI assistance.
I carried out the hardware prototyping, wiring, testing, troubleshooting,
PCB design, custom footprint creation, and enclosure design.
