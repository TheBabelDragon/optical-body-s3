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

## Clean calibration (whatsinthebox)

Boot path is now a scientifically clean isolation experiment:

```
1. Dark frame
   All emitters OFF → read BPW34 array → D[detector]

2. Formal ExcitationSequence (one-hot v1)
   For each source:
     fire source_id
     settle
     average N samples
     R_corrected = R_measured − D

3. OpticalFingerprint
   {
     dark_frame[],
     laser_response_matrix[][],   // dark-corrected
     timestamp,
     geometry_version
   }
   → FRAM (first physical memory)
```

Later sequence versions can change the experiment without touching hardware:
- v1: one-hot lasers
- v2: pairs
- v3: pseudo-random patterns

GPIO pin map stays last. Abstraction remains:

```
lasers.fire(id)
detectors.readAll(buf)
```

---

## Memory layers

```
FRAM (MB85RC256V)     → “Who am I?”          OpticalFingerprint + identity
MicroSD               → experience archive   JSONL logs, history
ESP32 RAM / PSRAM     → current thought      laser state, detector frame
```

MetaField owns episodic Field Memory (see metafield `MEMORY_ARCHITECTURE.md`).

---

## Firmware layout

```
src/
  main.cpp
  optical_body/
  drivers/          laser_matrix, bpw34_reader, mux_controller
  memory/           fram_identity, sd_archive
  calibration/
    excitation_sequence.*   ← formal experiment object
    optical_fingerprint.*   ← dark + matrix + version
    transfer_matrix.h
  protocol/         field_observation, json_encoder
```

---

## First “alive” behavior

```
Hello.
I am optical_s3_001
N emitters / M detectors
Dark frame — all emitters OFF
sequence=onehot-v1  steps=N
  source 0 → isolating… ok
  source 1 → isolating… ok
  …
OpticalFingerprint created + saved to FRAM.
```

That fingerprint is the first real field: emitter space × detector space.

---

## Hardware mapping (current plan)

**Laser side** — ESP32 GPIO → SN74AHCT125 → MOSFET → laser bank  
**Detector side** — BPW34 → ADS1115 (I2C) via CD74HC4067  
**Memory** — MB85RC256V FRAM, MicroSD, optional MCP23017 expansion

**Prefer real BPW34 values.** Synthetic only if `OPTICAL_USE_SYNTHETIC` is defined.

---

## Build

```bash
pio run -t upload
pio device monitor
```

Companion: [metafield](https://github.com/TheBabelDragon/metafield) — schemas, FieldMemoryStore, serial consumer.

---

*Part of the MetaField physical-field substrate work.*
