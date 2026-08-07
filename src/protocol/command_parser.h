#pragma once

#include <Arduino.h>

/**
 * Host → body commands (one line, Serial).
 *
 *   EXCITE <id>     fire that source once, measure, emit observation
 *   MAP             run full clean calibration
 *   VERIFY          run identity probe
 *   PASSIVE         resume cyclic passive loop (default)
 *   DUMP            print raw ADC volts (bring-up diagnostic)
 *
 * This is the hinge that closes the circle:
 * MetaField / active_probe decides → Aurora or host sends EXCITE → body shapes light.
 */
enum class BodyCommand : uint8_t {
  None = 0,
  Excite,
  Map,
  Verify,
  Passive,
  Dump,
};

struct ParsedCommand {
  BodyCommand type = BodyCommand::None;
  int         source_id = -1;
};

/** Non-blocking: read available Serial lines, parse first complete command. */
bool pollCommand(ParsedCommand& out);
