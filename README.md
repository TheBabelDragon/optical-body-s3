# optical-body-s3

**ESP32-S3 Optical Field Node for MetaField**

This is the *physical body*, not the intelligence.

```
ESP32-S3 Optical Body
        │
        │  excitation + measurement packets (FieldObservation)
        ↓
Aurora / MetaField / Echo Grid host
```

Implements **Field Body Protocol v0.1**  
(shared contract with the ultrasonic [Echo Body](https://github.com/TheBabelDragon/echo-grid-ultrasonic-os))

---

## Startup on Arch Linux

### 1. System packages

```bash
sudo pacman -S --needed git python python-pip python-virtualenv

# serial access without root
sudo usermod -aG dialout $USER
# log out and back in (or newgrp dialout)
```

### 2. PlatformIO CLI

```bash
# recommended: pipx (isolated)
sudo pacman -S --needed python-pipx
pipx ensurepath
pipx install platformio

# or: python -m venv ~/pio-venv && source ~/pio-venv/bin/activate && pip install platformio
```

Confirm:

```bash
pio --version
```

### 3. Clone + build

```bash
git clone https://github.com/TheBabelDragon/optical-body-s3.git
cd optical-body-s3

# first build downloads toolchain + libs (needs network)
pio run
```

### 4. Plug in ESP32-S3 + flash

```bash
# see the port
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null

# flash (auto-detects most S3 boards)
pio run -t upload

# serial monitor @ 115200
pio device monitor
# or: pio device monitor -p /dev/ttyACM0 -b 115200
```

Expected boot story:

```
optical-body-s3  —  MetaField body
cmds: EXCITE <id> | MAP | VERIFY | PASSIVE | DUMP
ui  : DKARDU OLED + EC11 enabled
… identity verify or first clean calibration …
passive loop
```

### 5. Host side (MetaField repo)

```bash
cd ../metafield   # or wherever you cloned it
source .venv/bin/activate
pip install pyserial

python optical_serial_consumer.py --port /dev/ttyACM0 --save /tmp/metafield/field_memory.jsonl
```

From another terminal you can shape light:

```bash
echo 'EXCITE 3' > /dev/ttyACM0
# or use active_probe.py --emit-command and paste the line
```

### Bring-up without detectors

In `platformio.ini`, uncomment:

```ini
-D OPTICAL_USE_SYNTHETIC=1
```

Then rebuild / upload. Packets still flow; ADC path is faked.

### Real ADC path (default)

The firmware drives a CD74HC4067 + ADS1115 by default.

- Mux S0–S3 → GPIO 4/5/6/7 (overridable)
- Mux EN → GPIO 15 (or tie to GND and set `MUX_EN_PIN=-1`)
- Mux SIG → ADS1115 A0
- ADS1115 ADDR → GND (address 0x48)

Once wired, type `DUMP` in the serial monitor to print raw volts for the first 8 channels.

### Pin map philosophy

**Pins are dictated at compile time**, not assigned interactively at runtime.

This keeps the body boring, deterministic, and safe. Edit `platformio.ini` (or a board-specific environment) and rebuild when the physical wiring changes. The boot log always prints the active map so there is never any ambiguity.

Example laser overrides:

```ini
-D LASER_PIN_0=10
-D LASER_PIN_1=11
; …
```

Any laser left at the default (`-1`) is treated as “not wired” and only logged.

---

## Local UI — DKARDU EC11 + 1.3″ SH1106 OLED

The firmware now supports the integrated **DKARDU** (or equivalent Estardyn-style) module:

- 1.3″ SH1106 OLED (I²C, address 0x3C)
- EC11 rotary encoder with push switch
- Confirm + Return buttons

**Default wiring** (see `HARDWARE_PINOUT.md` for full table):

| Signal          | GPIO |
|-----------------|------|
| OLED SDA / SCL  | 8 / 9 (shared I²C) |
| Encoder A / B   | 1 / 2 |
| Encoder SW      | 3 |
| Confirm         | 16 |
| Return          | 17 |
| VCC             | 3.3 V |

Enabled by default via `-D OPTICAL_UI=1`. Comment that flag out to build a pure headless node.

**Menu pages**

- **Status** — node ID, mode, streaming state
- **Identity** — trigger FRAM verify
- **Mode** — toggle Passive ↔ Held
- **Excite** — rotate to choose laser ID, Confirm to fire
- **Stream** — toggle continuous serial FieldObservation emission
- **Dump** — raw ADC snapshot
- **Calibrate** — full clean self-map

Rotate encoder to navigate / change values. Confirm (or encoder push) acts. Return always goes back to Status.

---

## Dual detector streams

```
BPW34
  +----> LM393   → "event happened"   (reflexes)
  +----> ADS1115 → "how much happened" (perception)
```

Full design: **[DETECTOR_ARCHITECTURE.md](DETECTOR_ARCHITECTURE.md)**

---

## Commands

```
EXCITE <id>   shape that source once
MAP           full clean calibration
VERIFY        identity probe vs FRAM
PASSIVE       resume cyclic scan
DUMP          print raw ADC volts (bring-up diagnostic)
```

(The same actions are also available from the OLED menu.)

---

## Clean calibration (whatsinthebox)

```
1. Dark frame (all emitters OFF) → D[detector]
2. ExcitationSequence one-hot → R_corrected = R − D
3. OpticalFingerprint → FRAM
```

On later boots: sparse `VERIFY` → remap only on drift.

---

## Sibling / host docs

- [metafield](https://github.com/TheBabelDragon/metafield) — schemas, FieldMemoryStore, `active_probe.py`
- [BOM_AND_MILESTONE.md](BOM_AND_MILESTONE.md) — frozen BOM + acceptance target
- Echo body: [echo-grid-ultrasonic-os](https://github.com/TheBabelDragon/echo-grid-ultrasonic-os)

---

*Part of the MetaField physical-field substrate work.*
