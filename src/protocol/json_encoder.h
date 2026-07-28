#pragma once

#include <Arduino.h>
#include "field_observation.h"

/**
 * Encode a FieldObservation into a compact JSON string
 * that matches the spirit of the Python schema.
 */
String encodeFieldObservation(const FieldObservation& obs);
