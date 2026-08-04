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
cmds: EXCITE <id> | MAP | VERIFY | PASSIVE
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
```

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
