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

## Protocol compatibility

Every observation is emitted in **two** forms:

1. **Rich JSON** (original optical format, full detector array + anomaly + event_mask)
2. **Compact `OBS {...}` line** (Field Body Protocol v0.1) so any host that speaks the shared contract can close the loop

Commands remain:

```
EXCITE <id>
MAP
VERIFY
PASSIVE
```

---

## Clean calibration (whatsinthebox)

```
1. Dark frame (all emitters OFF) → D[detector]
2. ExcitationSequence one-hot → R_corrected = R − D
3. OpticalFingerprint → FRAM
```

---

## Memory layers

```
FRAM (MB85RC256V)     → “Who am I?”          OpticalFingerprint + identity
MicroSD               → experience archive   JSONL logs, history
ESP32 RAM / PSRAM     → current thought      laser state, detector frame
```

---

## Firmware layout

```
src/
  main.cpp
  optical_body/
  drivers/
  memory/
  calibration/
  protocol/
```

---

## Build

```bash
pio run -t upload
pio device monitor
```

Sibling body (ultrasonic): [echo-grid-ultrasonic-os](https://github.com/TheBabelDragon/echo-grid-ultrasonic-os)

---

*Part of the MetaField physical-field substrate work.*
