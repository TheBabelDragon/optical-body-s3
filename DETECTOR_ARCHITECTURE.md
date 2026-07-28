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

Use for:

- threshold crossings
- beam interruption
- sudden changes
- timing events
- fast pattern detection

Example:

```
Laser 7 ON
BPW34 rises above threshold
LM393 → EVENT = 1
```

MetaField gets a clean **“something changed”** signal without waiting on a full ADC scan.

### ADS1115 = analog field layer (perception)

Use for:

- calibration / dark frame
- transfer matrix / OpticalFingerprint
- intensity mapping
- gradual changes
- confidence and anomaly

Example:

```
Laser 7 ON
Detector 42: 0.713
Detector 43: 0.284
Detector 44: 0.052
```

This is the optical fingerprint itself.

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

- **ADCs characterize everything** (full field, calibration, geometry).
- **Comparators watch important regions** (not necessarily every diode at first).

---

## Realistic first build (100 BPW34)

### Analog measurement

```
100 BPW34
    |
10× CD74HC4067
    |
4× ADS1115
    |
I2C → ESP32-S3
```

### Event measurement (selected / high-value channels)

```
BPW34 groups (subset)
    |
LM393 comparators
    |
GPIO and/or MCP23017
    |
ESP32-S3
```

You do **not** need one LM393 per diode immediately. Start with a sparse event mask over interesting faces / clusters; expand later.

---

## Resource impact on ESP32-S3

```
I2C
 ├── ADS1115 ×4
 ├── FRAM (MB85RC256V)
 ├── MCP23017 (optional expansion)
GPIO
 ├── MUX select lines (CD74HC4067)
 ├── LM393 event inputs (sparse)
 ├── laser control / status
```

Manageable. No need to burn the whole GPIO bank on events day one.

---

## Biological analogy (intentional)

| Layer        | Hardware     | Role              |
|--------------|--------------|-------------------|
| Reflexes     | LM393        | fast events       |
| Perception   | ADS1115      | analog field      |
| Memory       | FRAM + SD    | identity + archive|
| Interpretation | MetaField  | meaning           |
| Coordination | Aurora       | experiments       |

Fast reactions without sacrificing the high-resolution data needed to learn geometry.

---

## Firmware mapping

| Stream  | Module                         | Output                          |
|---------|--------------------------------|---------------------------------|
| Analog  | `drivers/bpw34_reader`         | float vector 0..1               |
| Event   | `drivers/event_reader`         | bitmask / list of active edges  |
| Combined| `FieldObservation` + optional `events[]` in modality | host / MetaField |

Calibration (dark frame, transfer matrix, OpticalFingerprint) uses **analog only**.  
Passive loop and future adaptive probing can fuse both streams.

---

*Living document. Dual streams strengthen the physical field interface.*
