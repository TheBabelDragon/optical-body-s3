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
sudo pacman -S --needed python-pipx
pipx ensurepath
pipx install platformio
```

Confirm:

```bash
pio --version
```

### 3. Clone + build

```bash
git clone https://github.com/TheBabelDragon/optical-body-s3.git
cd optical-body-s3
pio run
```

### 4. Plug in ESP32-S3 + flash

```bash
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
pio run -t upload
pio device monitor
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
cd ../metafield
source .venv/bin/activate
pip install pyserial

python optical_serial_consumer.py --port /dev/ttyACM0 --save /tmp/metafield/field_memory.jsonl
```

From another terminal you can shape light:

```bash
echo 'EXCITE 3' > /dev/ttyACM0
```

### Real ADC path (required)

There is no synthetic detector mode. If ADS1115 is missing, the body stays silent.

The firmware drives a CD74HC4067 + ADS1115.

- Mux S0–S3 → GPIO 4/5/6/7 (overridable)
- Mux EN → GPIO 15 (or tie to GND and set `MUX_EN_PIN=-1`)
- Mux SIG → ADS1115 A0
- ADS1115 ADDR → GND (address 0x48)

Once wired, type `DUMP` in the serial monitor to print raw volts for the first 8 channels.

### Pin map philosophy

**Pins are dictated at compile time**, not assigned interactively at runtime.

Edit `platformio.ini` and rebuild when the physical wiring changes. The boot log always prints the active map.

Example laser overrides:

```ini
-D LASER_PIN_0=10
-D LASER_PIN_1=11
```

Any laser left at the default (`-1`) is treated as “not wired” and only logged.

---

## Local UI — DKARDU EC11 + 1.3″ SH1106 OLED

Enabled by default via `-D OPTICAL_UI=1`. See `HARDWARE_PINOUT.md`.

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

---

*Part of the MetaField physical-field substrate work.*
