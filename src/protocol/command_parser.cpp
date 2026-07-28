#include "command_parser.h"

bool pollCommand(ParsedCommand& out) {
  out.type = BodyCommand::None;
  out.source_id = -1;

  if (!Serial.available()) return false;

  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return false;

  if (line.equalsIgnoreCase("MAP")) {
    out.type = BodyCommand::Map;
    return true;
  }
  if (line.equalsIgnoreCase("VERIFY")) {
    out.type = BodyCommand::Verify;
    return true;
  }
  if (line.equalsIgnoreCase("PASSIVE")) {
    out.type = BodyCommand::Passive;
    return true;
  }
  if (line.startsWith("EXCITE") || line.startsWith("excite")) {
    int sp = line.indexOf(' ');
    if (sp < 0) return false;
    int id = line.substring(sp + 1).toInt();
    out.type = BodyCommand::Excite;
    out.source_id = id;
    return true;
  }

  Serial.print(F("[CMD] unknown: "));
  Serial.println(line);
  return false;
}
