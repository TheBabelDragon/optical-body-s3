# Phase-0 wiring lock (dictated)

Wire exactly this. Pins are compile-time defaults in `platformio.ini`.

## ESP32-S3 pin map

| Function | GPIO | Goes to |
|----------|------|---------|
| I²C SDA | **8** | ADS1115 SDA, MCP23017 SDA, (PCA9548A SDA, INA219 SDA) |
| I²C SCL | **9** | ADS1115 SCL, MCP23017 SCL, (PCA9548A SCL, INA219 SCL) |
| Mux S0 | **4** | CD74HC4067 S0 |
| Mux S1 | **5** | CD74HC4067 S1 |
| Mux S2 | **6** | CD74HC4067 S2 |
| Mux S3 | **7** | CD74HC4067 S3 |
| Mux EN | **15** | CD74HC4067 EN (active LOW) |
| CAN SPI SCK | **12** | MCP2518FD SCK |
| CAN SPI MOSI | **11** | MCP2518FD MOSI/SDI |
| CAN SPI MISO | **13** | MCP2518FD MISO/SDO |
| CAN SPI CS | **10** | MCP2518FD CS/nCS |
| CAN SPI INT | **14** | MCP2518FD INT |

Power: **3.3 V** and **GND** to every module (ADS, mux, MCP23017, MCP2518FD as required by that board).

---

## Analog path (16 detectors)

```
BPW34 (or sensor) bank
        │
        ▼
CD74HC4067  C0 … C15
        │
       SIG ──────────────────► ADS1115 A0
        │
   S0 S1 S2 S3 EN  ◄── GPIO 4 5 6 7 15

ADS1115
  ADDR → GND          (I²C address 0x48)
  SDA  → GPIO 8
  SCL  → GPIO 9
  VDD  → 3.3 V
  GND  → GND
```

---

## Event path (LM393 → MCP23017)

```
LM393 OUT → MCP23017 PA0 … (one OUT per channel)
LM393 G   → GND
LM393 VD  → 3.3 V

MCP23017
  SDA/SCL → GPIO 8 / 9
  VCC → 3.3 V
  GND → GND
  Address: A0 A1 A2 → GND  ⇒ 0x20
```

Pull-ups: use MCP23017 internal pull-ups (firmware enables them).

---

## Field Bus (CAN-FD)

```
ESP32 SPI (10–14) → MCP2518FD module
MCP2518FD CANH/CANL → twisted pair → SH-C31G host
120 Ω at both ends of the bus
```

---

## I²C address map (Phase 0)

| Device | Address | Notes |
|--------|---------|-------|
| ADS1115 #0 | **0x48** | ADDR→GND |
| MCP23017 | **0x20** | A0–A2→GND |
| PCA9548A (optional) | 0x70 | when you need more I²C branches |
| INA219 (optional) | 0x40 | default |
| FRAM (existing) | 0x50 | already in firmware |

---

## Bring-up order

1. Power + I²C only (ADS @ 0x48) → serial `DUMP`
2. Add mux S0–S3/EN/SIG → `DUMP` again, change light on C0
3. Add MCP23017 + one LM393 → watch event mask later
4. Add MCP2518FD + SH-C31G → `NODE_HELLO` / heartbeat on CAN-FD

---

*Do not reassign pins interactively. Change `platformio.ini` and rebuild if the physical map must change.*
