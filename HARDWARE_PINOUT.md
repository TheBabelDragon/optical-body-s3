# Hardware pinout reference

Taken from the physical modules on the desk. Use this when wiring — silkscreen order matches the boards.

---

## CD74HC4067 — 16-channel analog mux

**Channel side:** `C0 C1 C2 … C15`

**Control side (in order):**

| Pin | Function |
|-----|----------|
| SIG | Common analog I/O → ADS1115 Ax |
| S3  | Select bit 3 |
| S2  | Select bit 2 |
| S1  | Select bit 1 |
| S0  | Select bit 0 |
| EN  | Enable (active LOW) |
| VCC | 3.3 V / 5 V |
| GND | Ground |

Firmware defaults: S0–S3 → GPIO 4/5/6/7, EN → GPIO 15 (overridable).

---

## ADS1115 — 16-bit I²C ADC + PGA

| Pin  | Function |
|------|----------|
| VDD  | 3.3 V |
| GND  | Ground |
| SCL  | I²C clock |
| SDA  | I²C data |
| ADDR | Address select (→ GND = 0x48, VDD = 0x49, SDA = 0x4A, SCL = 0x4B) |
| ALRT | Alert / ready (optional) |
| A0   | Analog in 0 (typical mux SIG target) |
| A1   | Analog in 1 |
| A2   | Analog in 2 |
| A3   | Analog in 3 |

---

## LM393 — dual comparator (event / threshold)

| Side | Pins |
|------|------|
| Power | **VD**, **G** |
| Opposite | 3-pin terminal (channel inputs — match your board silkscreen) |
| Output breakout | **OUT**, **G** (open-collector style → MCP23017 or GPIO with pull-up) |

Wire OUT → MCP23017 port pin (or direct GPIO). Active level depends on how the front-end is biased; EventReader assumes active-LOW with pull-ups.

---

## PCA9548A — 8-channel I²C mux

**Host / control side (in order):**

| Pin | Function |
|-----|----------|
| VIN | Power |
| GND | Ground |
| SDA | Host I²C data |
| SCL | Host I²C clock |
| RST | Reset (active LOW; pull high if unused) |
| A0  | Address bit 0 |
| A1  | Address bit 1 |
| A2  | Address bit 2 |

**Channel side:** eight pairs `SD0/SC0` … `SD7/SC7` (downstream SDA/SCL).

Use this to park multiple ADS1115 (or other same-address devices) on separate channels when ADDR pins alone aren’t enough.

---

## MCP23017 — 16-bit I²C GPIO expander

**Port B side:** `PB0 PB1 PB2 PB3 PB4 PB5 PB6 PB7` · GND · VCC  
**Port A side:** `PA7 PA6 PA5 PA4 PA3 PA2 PA1 PA0` · GND · VCC  

**I²C / interrupt header:**

| Pin  | Function |
|------|----------|
| INTB | Interrupt B |
| INTA | Interrupt A |
| SCL  | I²C clock |
| SDA  | I²C data |
| GND  | Ground |
| VCC  | 3.3 V |

Default EventReader target: MCP23017 @ 0x20, PA0–PA7 / PB0–PB7 as LM393 inputs with pull-ups.

---

## INA219 — DC current / power sensor

| Pin    | Function |
|--------|----------|
| VCC    | 3.3 V / 5 V |
| GND    | Ground |
| SCL    | I²C clock |
| SDA    | I²C data |
| VIN−   | Load-side / shunt − |
| VIN+   | Supply-side / shunt + |

(Shunt is typically onboard; VIN+/VIN− are the high-side measurement points.)

---

## MCP2518FD — CAN-FD module (ordered)

SPI to ESP32-S3. Default firmware map (override in `platformio.ini`):

| Function | GPIO |
|----------|------|
| SCK  | 12 |
| MOSI | 11 |
| MISO | 13 |
| CS   | 10 |
| INT  | 14 |

CANH / CANL → bus. Terminate both ends of the bus with 120 Ω.

---

## Suggested minimal optical bring-up chain

```
BPW34 bank → CD74HC4067 C0..C15
CD74HC4067 SIG → ADS1115 A0
ADS1115 SDA/SCL → ESP32 I²C (or via PCA9548A channel)
ADS1115 ADDR → GND (0x48)
Mux S0..S3 → GPIO 4..7
Mux EN → GPIO 15 (or GND)

LM393 OUT → MCP23017 PAx (pull-up)
MCP23017 SDA/SCL → same I²C bus

MCP2518FD SPI → GPIO 10–14
MCP2518FD CANH/CANL → Field Bus + SH-C31G host
```

---

*Update this file if a board revision changes silkscreen order.*
