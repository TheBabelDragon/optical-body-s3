# optical-body-s3

**ESP32-S3 Optical Field Node for MetaField**

This is the *physical body*, not the intelligence.

```
ESP32-S3 Optical Body
        │
        │  excitation + measurement packets (FieldObservation)
        ↓
Aurora coordination
        ↓
MetaField core  (geometry, attractors, confidence, curiosity)
```

---

## Dual detector streams

LM393 and ADS1115 are two views of the same BPW34 field — not redundant.

```
BPW34
  +----> LM393   → "event happened"   (reflexes)
  +----> ADS1115 → "how much happened" (perception)
```

- **ADS1115** — calibration, transfer matrix, intensity, confidence  
- **LM393** — threshold crossings, sudden change, timing (sparse OK)

Full design: **[DETECTOR_ARCHITECTURE.md](DETECTOR_ARCHITECTURE.md)**

---

## Clean calibration (whatsinthebox)

```
1. Dark frame (all emitters OFF) → D[detector]
2. ExcitationSequence one-hot → R_corrected = R − D
3. OpticalFingerprint → FRAM
```

Calibration uses **analog only**. Events are reflexes for the live loop.

GPIO pin map stays last. Abstraction remains `lasers.fire(id)` / `detectors.readAll(buf)` / `events.readMask()`.

---

## Memory layers

```
FRAM (MB85RC256V)     → “Who am I?”          OpticalFingerprint + identity
MicroSD               → experience archive   JSONL logs, history
ESP32 RAM / PSRAM     → current thought      laser state, detector frame
```

See metafield `MEMORY_ARCHITECTURE.md`.

---

## Firmware layout

```
src/
  main.cpp
  optical_body/
  drivers/
    laser_matrix.*
    bpw34_reader.*      ← ADS1115 analog
    event_reader.*      ← LM393 events (sparse)
    mux_controller.*
  memory/               fram_identity, sd_archive
  calibration/          excitation_sequence, optical_fingerprint
  protocol/             field_observation (+ event_mask), json_encoder
```

---

## Hardware sketch (100 BPW34)

```
Analog:  100 BPW34 → 10× CD74HC4067 → 4× ADS1115 → I2C → S3
Events:  selected BPW34 groups → LM393 → GPIO/MCP23017 → S3
Memory:  MB85RC256V FRAM, MicroSD
Lasers:  GPIO → SN74AHCT125 → MOSFET → bank
```

---

## Build

```bash
pio run -t upload
pio device monitor
```

Companion: [metafield](https://github.com/TheBabelDragon/metafield)

---

*Part of the MetaField physical-field substrate work.*
