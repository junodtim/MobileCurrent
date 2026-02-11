ESP32-S3 Dual ADS1115 Sensor System

Functional Specification Document (FSD)

1. Overview

This document defines the functional, electrical, and wiring specifications for an ESP32‑S3–based sensing system used in an RV environment. The system integrates:

Two ADS1115 16‑bit ADC modules

ZMPT101B AC voltage sensors

SCT‑013 current sensors

I²C communication bus

Stable power‑up circuitry including pull‑ups, decoupling capacitors, and CHIP_PU RC delay

The goal is to ensure reliable measurement, clean power delivery, and stable boot behavior.

2. System Architecture

2.1 High‑Level Block Diagram

ESP32‑S3 (main controller)

Provides 3.3 V logic and I²C master

Reads voltage and current data from ADS1115 modules

ADS1115 #1 (Voltage sensing)

Connected to ZMPT101B modules

I²C address: 0x48

ADS1115 #2 (Current sensing)

Connected to SCT‑013 sensors

I²C address: 0x49

Power subsystem

5 V input → ESP32 VIN

ESP32 3V3 output → ADS1115 VDD

Decoupling capacitors on VIN and ADS1115 modules

CHIP_PU RC delay for stable boot

3. Electrical Specifications

3.1 Power Input

Source

Voltage

Notes

USB‑C / Micro‑USB

5 V

Easiest method; powers onboard regulator

VIN pin

5 V regulated

Add bulk capacitors at VIN

3V3 pin

3.3 V regulated

Must be clean and stable; bypasses onboard LDO

3.2 Power Conditioning Requirements

VIN bulk capacitors:

47 µF electrolytic

100 nF ceramic

ADS1115 VDD decoupling:

10 µF electrolytic

100 nF ceramic

Placement: As close as possible to each module’s power pins.

3.3 ESP32 CHIP_PU (EN) Circuit

Component

Value

Purpose

Pull‑up resistor

10 kΩ → 3.3 V

Keeps ESP32 enabled

Capacitor

100 nF (up to 1 µF optional)

RC delay for stable boot

Reset button

Optional

Pulls CHIP_PU to GND

RC Delay Formula: [ V(t) = V_{CC} \times (1 - e^{-t/RC}) ]

R = 10 kΩ

C = 100 nF → ~1 ms delay

C = 1 µF → ~10 ms delay

4. I²C Bus Specifications

4.1 Bus Voltage

All I²C devices operate at 3.3 V logic.

No level shifters required.

4.2 Pull‑Up Resistors

Line

Resistor

Notes

SDA

4.7 kΩ → 3.3 V

Recommended

SCL

4.7 kΩ → 3.3 V

Recommended

Range: 3.3 kΩ (strong) to 10 kΩ (weak)

Only one set of pull‑ups is required for the entire bus.

4.3 I²C Addresses

Module

Address Pin

Address

ADS1115 #1

ADDR → GND

0x48

ADS1115 #2

ADDR → VDD

0x49

4.4 Recommended Bus Speed

Default: 100 kHz

Optional: 400 kHz Fast Mode (if wiring is short and clean)

5. Sensor Wiring Specifications

5.1 ZMPT101B Voltage Sensors → ADS1115 #1

Sensor

ADS1115 Channel

ZMPT #1

A0

ZMPT #2

A1

ZMPT #3

A2

5.2 SCT‑013 Current Sensors → ADS1115 #2

Sensor

ADS1115 Channel

SCT‑013 #1

A0

SCT‑013 #2

A1

SCT‑013 #3

A2

Notes:

Some SCT‑013 models require an external burden resistor.

ZMPT101B modules require calibration for accurate AC measurement.

6. ESP32‑S3 Wiring Summary

6.1 I²C Pins

ESP32‑S3 Pin

Function

GPIO 8

SDA

GPIO 9

SCL

6.2 Power Pins

ESP32 Pin

Destination

3V3

ADS1115 VDD

GND

All modules

VIN

5 V input

7. Firmware Requirements

7.1 ADS1115 Configuration

Gain: GAIN_ONE (±4.096 V range)

Mode: Single‑ended reads

Sampling: 860 SPS recommended for AC waveform sampling

7.2 Example Firmware Structure

Initialize I²C

Initialize both ADS1115 modules

Read voltage channels

Read current channels

Apply calibration factors

Output via serial or Wi‑Fi

8. Reliability & Stability Requirements

8.1 Power Stability

Bulk capacitors required at VIN

Decoupling capacitors required at each ADS1115

8.2 Boot Stability

CHIP_PU pull‑up + RC delay mandatory

Keep capacitor ground trace short

8.3 Noise Reduction

Keep sensor wiring short

Avoid routing I²C near AC lines

Use twisted pair for SCT‑013 if possible

9. Mechanical & Installation Notes

Secure ZMPT101B modules to avoid AC interference

Clamp SCT‑013 around only one conductor (hot or neutral)

Keep ADS1115 modules close to ESP32 to minimize I²C capacitance

10. Revision History

Version

Date

Notes

1.0

Feb 2026

Initial FSD generated from wiring and power design documents

11. Appendix

11.1 Recommended Wiring Diagram (Text Form)

5V → VIN (ESP32)
   └─ 47µF + 100nF → GND

ESP32 3V3 → ADS1115 #1 VDD
ESP32 3V3 → ADS1115 #2 VDD

GPIO 8 (SDA) → Both ADS1115 SDA
GPIO 9 (SCL) → Both ADS1115 SCL

3.3V → 4.7kΩ → SDA
3.3V → 4.7kΩ → SCL

ADS1115 #1 ADDR → GND (0x48)
ADS1115 #2 ADDR → VDD (0x49)

CHIP_PU → 10kΩ → 3.3V
CHIP_PU → 100nF → GND

This FSD consolidates all wiring, electrical, and operational requirements into a single authoritative reference for your ESP32‑S3 RV sensor system.