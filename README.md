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

MetaField’s job (elsewhere):
- Infer geometry
- Learn transfer behavior
- Maintain attractors
- Calculate confidence / anomaly
- Decide what is interesting

Companion docs in [metafield](https://github.com/TheBabelDragon/metafield):
- `PHYSICAL_FIELD_SUBSTRATE.md`
- `schemas/field_observation.py`
- Issue #1 (Phase 0)

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

**Expansion**
```
MCP23017 — laser enables, status lines, calibration controls
```

FRAM (if present) for persistent geometry fingerprint / identity.

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
    ads1115_manager.cpp / .h
    mux_controller.cpp / .h
  calibration/
    excitation_sequence.cpp / .h
    transfer_matrix.cpp / .h
  protocol/
    field_observation.cpp / .h
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
```

That fingerprint (`M[laser][detector]`) is the optical identity of the structure.

---

## Build

PlatformIO project. Target: ESP32-S3.

```bash
pio run -t upload
pio device monitor
```

---

*Part of the MetaField physical-field substrate work.*
