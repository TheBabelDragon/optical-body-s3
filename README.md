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

The S3’s job:
- Drive lasers
- Sample BPW34 detectors (**real ADC values preferred**)
- Timestamp observations
- Report health
- Execute calibration / self-map sequences
- Maintain local identity (node_id + geometry fingerprint)
- Own working RAM + FRAM identity + (optional) MicroSD experience archive

MetaField’s job (elsewhere):
- Infer geometry
- Learn transfer behavior
- Maintain attractors + **Field Memory** (episodic)
- Calculate confidence / anomaly
- Decide what is interesting

Companion docs in [metafield](https://github.com/TheBabelDragon/metafield):
- `PHYSICAL_FIELD_SUBSTRATE.md`
- `MEMORY_ARCHITECTURE.md` ← three-tier memory
- `schemas/field_observation.py`
- `schemas/field_memory.py`
- Issue #1 (Phase 0)

---

## Memory layers (this node owns two of them)

```
FRAM (MB85RC256V)     → “Who am I?”          identity + calibration
MicroSD               → experience archive   JSONL logs, history
ESP32 RAM / PSRAM     → current thought      laser state, detector frame
```

MetaField owns the third layer (episodic Field Memory).

See `MEMORY_ARCHITECTURE.md` in the metafield repo for the full rationale.

---

## Phase 0 goal

Match the Python stub (`optical_body_stub.py` in metafield) in spirit:

Emit valid observation packets that can be consumed by the same schema.

**Prefer real BPW34 → ADS1115 values from day one.**  
Synthetic values only as a compile-time fallback during board bring-up.

---

## Hardware mapping (current plan)

**Laser side**
```
ESP32 GPIO → SN74AHCT125 → MOSFET → laser bank
```

**Detector side**
```
BPW34 → ADS1115 (I2C) via CD74HC4067 mux
        (optional parallel LM393 event path for fast triggers)
```

**Expansion / memory**
```
MCP23017     — laser enables, status lines, calibration controls
MB85RC256V   — FRAM identity (persistent calibration)
MicroSD      — experience archive (JSONL)
```

---

## Firmware layout

```
src/
  main.cpp
  optical_body/
    optical_body.cpp / .h
  drivers/
    laser_matrix.cpp / .h
    bpw34_reader.cpp / .h
    mux_controller.cpp / .h
  memory/
    fram_identity.cpp / .h     ← persistent “Who am I?”
    sd_archive.cpp / .h        ← experience archive
  calibration/
    transfer_matrix.cpp / .h
  protocol/
    field_observation.h
    json_encoder.cpp / .h
```

---

## First “alive” behavior

```
Hello.
I am optical_s3_001
N emitters / M detectors
Beginning self-map…
Laser 0 → recording…
Laser 1 → recording…
…
Geometry fingerprint created.
[FRAM] identity saved
```

That fingerprint (`M[laser][detector]`) is the optical identity of the structure and is intended to live in FRAM.

---

## Build

PlatformIO project. Target: ESP32-S3.

```bash
pio run -t upload
pio device monitor
```

Optional: define `OPTICAL_USE_SYNTHETIC` in `platformio.ini` only when detectors are not yet wired.

---

*Part of the MetaField physical-field substrate work.*
