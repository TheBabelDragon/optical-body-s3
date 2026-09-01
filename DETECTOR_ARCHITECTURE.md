# Detector Architecture — Two Views of the Same Field

LM393 and ADS1115 are **not redundant**. They are two interpretations of each BPW34 channel.

```
BPW34
  |
  +----> LM393  ----> "event happened"     (fast / reflex)
  |
  +----> ADS1115 --> "how much happened"   (analog / perception)
```

---

## Role separation

### LM393 = fast event layer (reflexes)

Use for threshold crossings, beam interruption, sudden changes, timing events.

### ADS1115 = analog field layer (perception)

Use for calibration / dark frame, transfer matrix / OpticalFingerprint, intensity mapping, gradual changes, confidence and anomaly.

---

## Dark track (isolation memory)

Dark is not a single subtracted snapshot. Emitters OFF is a **voltage stance**.
Per-detector `q` integrates `(raw_dark - baseline)` and leaks:

```
HOLD   → quiet leak, isolation OK
CHARGE → residual still rising after light (traps / mux leftover)
RELAX  → q decaying toward baseline
FAULT  → leak or offset too large
```

One-hot isolation waits in `allOff` until HOLD/RELAX (or marks `health=partial`).
Optical rows subtract `baseline + q`, not the frozen cal frame alone.

Firmware: `src/calibration/dark_track.*` used by `OpticalBody::isolateDark`.

This is memristor-shaped software on the diode leak. Not a discrete memristor IC.
Do not put that state on the bias rail that sets `C_d`.

---

## Parallel nervous system

```
                 ESP32-S3
                    |
     +--------------+--------------+
     |                             |
Event stream                  Analog stream
     |                             |
  LM393 bank                  ADS1115 bank
     |                             |
"what changed?"            "what is the state?"
     |                             |
     +-------------+---------------+
                   |
              MetaField / Aurora
```

---

## Firmware mapping

| Stream  | Module                         | Output                          |
|---------|--------------------------------|---------------------------------|
| Analog  | `drivers/bpw34_reader`         | float vector 0..1               |
| Event   | `drivers/event_reader`         | bitmask / list of active edges  |
| Dark    | `calibration/dark_track`       | HOLD/CHARGE/RELAX/FAULT + q     |
| Combined| `FieldObservation`             | host / MetaField                |

Calibration uses analog + dark track. Passive loop fuses both streams.

---

*Living document. Dual streams strengthen the physical field interface.*
