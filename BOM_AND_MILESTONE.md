# BOM (final) + First Milestone

**Stop adding ICs.** The architecture has the important pieces.

The only bulk-memory item still worth having before calling the cart final:

✅ **MicroSD breakout (SPI)** — experience archive

```
ESP32-S3 RAM          → current state
FRAM MB85RC256V       → identity + calibration
MicroSD               → experience archive
MetaField memory      → meaning / replay
```

---

## Final hardware list

| Category | Items |
|----------|--------|
| Compute | ESP32-S3 (PinPulse / DevKit; PSRAM optional for 2nd node) |
| Emitters | ~40 lasers + SN74AHCT125 buffers + MOSFET switching |
| Detectors | ~100 BPW34 |
| Analog | ADS1115 bank + CD74HC4067 mux routing |
| Events | LM393 layer (sparse OK) |
| Expansion | MCP23017 |
| Memory | MB85RC256V FRAM + **MicroSD** |
| Interconnect | JST / terminals / wiring |
| Passives | capacitor / resistor / diode kits |
| Mechanical | enclosure / dodeca (whatsinthebox) hardware |

No further ICs required for Phase 0–1.

---

## First milestone (acceptance)

> **The device can reboot, identify its optical body, and reproduce the same field response.**

**Firmware boot path (implemented):**

```
begin()
  load FRAM identity if present
if identity:
  verifyIdentity()   # dark + sparse probe vs expected
  residual ≤ threshold → "Body unchanged" → skip full map
  residual > threshold → "Geometry drift detected" → runSelfMap()
else:
  runSelfMap()       # first birth certificate
passive loop
```

Concrete checks:

1. **Boot** — load OpticalFingerprint from FRAM (or calibrate if empty).
2. **Identify** — geometry_version present; sparse probe against expected.
3. **Reproduce** — residual within threshold ⇒ body unchanged.
4. **Persist** — power cycle; identity still loads; remap only on drift.
5. **Archive** (once SD wired) — verify + calibration JSONL on card.

When that works on hardware, MetaField has something real to learn from.

---

## What not to do next

- Do not add more sensor ICs “just in case.”
- Do not expand LM393 to 100 channels before sparse events prove useful.
- Do not redesign the pin map until the first calibration dataset exists.

Next work is **measurement**, not parts.

---

*Architecture complete. Dataset is the next deliverable.*
