#pragma once

#include <Arduino.h>

/**
 * SdArchive — experience archive (not working memory)
 *
 * Purpose:
 *   - thousands / millions of optical experiments
 *   - JSONL observation streams
 *   - calibration history
 *   - replay datasets for MetaField offline work
 *
 * Not fast. Not for the current thought.
 * FRAM = identity, SD = experience, RAM = current thought.
 */
class SdArchive {
public:
  bool begin(uint8_t cs_pin = 5);

  /** Append one observation JSON line. */
  bool appendObservation(const String& json_line);

  /** Append a calibration event (self-map finished, etc.). */
  bool appendCalibrationEvent(const String& json_line);

  /** Simple status. */
  bool isReady() const { return ready_; }
  uint32_t observationCount() const { return obs_count_; }

private:
  bool ready_ = false;
  uint32_t obs_count_ = 0;
  // TODO: SdFat / SD.h open of /optical/obs.jsonl and /optical/cal.jsonl
};
