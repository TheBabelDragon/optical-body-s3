#include "optical_fingerprint.h"
#include <string.h>
#include <math.h>

void OpticalFingerprint::clear() {
  for (int d = 0; d < MAX_DETECTORS; ++d)
    dark_frame[d] = 0.0f;
  for (int l = 0; l < MAX_LASERS; ++l)
    for (int d = 0; d < MAX_DETECTORS; ++d)
      matrix[l][d] = 0.0f;
  num_lasers = 0;
  num_detectors = 0;
  timestamp_ms = 0;
  strncpy(geometry_version, "uninitialized", sizeof(geometry_version) - 1);
  valid = false;
}

void OpticalFingerprint::setDark(const float* d, uint8_t n) {
  if (!d) return;
  if (n > MAX_DETECTORS) n = MAX_DETECTORS;
  num_detectors = n;
  for (uint8_t i = 0; i < n; ++i)
    dark_frame[i] = d[i];
}

void OpticalFingerprint::setRow(uint8_t laser, const float* row, uint8_t n) {
  if (!row || laser >= MAX_LASERS) return;
  if (n > MAX_DETECTORS) n = MAX_DETECTORS;
  if (laser + 1 > num_lasers) num_lasers = laser + 1;
  if (n > num_detectors) num_detectors = n;
  for (uint8_t i = 0; i < n; ++i)
    matrix[laser][i] = row[i];
}

float OpticalFingerprint::expected(uint8_t laser, uint8_t detector) const {
  if (laser >= num_lasers || detector >= num_detectors) return NAN;
  return matrix[laser][detector];
}
