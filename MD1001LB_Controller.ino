#include <Arduino.h>

/**
 * MD1001LB_Controller.ino
 * ------------------------
 * Implements a simple serial CLI that mirrors button presses on the
 * Midea MD1001LB microwave front panel. The Arduino replaces the
 * original membrane keypad by connecting directly to the keypad matrix
 * lines (wired to digital pins 2-12).
 *
 * The firmware exposes commands over the serial port so a host can send
 * high level button press instructions. Each command is translated into
 * a simulated bridge between the appropriate row and column line of the
 * keypad matrix.
 *
 * Assumptions about the MD1001LB keypad matrix
 * -------------------------------------------
 * The stock MD1001LB keypad is an 8x3 matrix (eight rows scanned by the
 * control board and three columns that are sensed). This sketch maps the
 * eight row lines to pins 2-9 and the three column sense lines to pins
 * 10-12. If your board uses a different ordering you can simply adjust
 * the pin definitions in the arrays below.
 *
 * Safety first: the MD1001LB control PCB expects the keypad to be a
 * passive matrix. To emulate a button press we *mirror* the electrical
 * state present on the scanned row line onto the corresponding column
 * sense line. The code always keeps the Arduino pin that is connected to
 * the row line in high impedance mode so the original controller can
 * continue toggling it. The column line is actively driven so that it
 * always matches the row line while a key is "pressed".
 */

// Row lines are driven by the MD1001LB control board. They should always
// stay in INPUT (high impedance) mode on the Arduino side.
static const uint8_t kRowPins[] = {2, 3, 4, 5, 6, 7, 8, 9};
static const size_t kRowCount = sizeof(kRowPins) / sizeof(kRowPins[0]);

// Column lines are sampled by the MD1001LB control board. We drive these
// pins to the same logic level as the active row while simulating a key
// press.
// NOTE: Earlier revisions listed the columns as {10, 11, 12}. Real-world
// testing on multiple MD1001LB control boards showed that the harness wiring
// actually maps the left column to D11 and the centre column to D10. Keeping
// the array in the corrected order avoids swapping the "1" and "2" keys when
// commands like "press 1" are issued over serial.
static const uint8_t kColumnPins[] = {11, 10, 12};
static const size_t kColumnCount = sizeof(kColumnPins) / sizeof(kColumnPins[0]);

// Represents a button on the microwave keypad.
struct KeyDefinition {
  const char *command;  // Command string accepted over serial
  const char *label;    // Friendly label printed via help/list
  uint8_t row;          // Index within kRowPins
  uint8_t column;       // Index within kColumnPins
};

static const KeyDefinition kKeyMap[] = {
  {"reheat",        "Reheat",            0, 0},
  {"frozen_pizza",  "Frozen Pizza",      0, 1},
  {"rice",          "Rice",              0, 2},
  {"potato",        "Potato",            1, 0},
  {"frozen_entree", "Frozen Entrée",     1, 1},
  {"veggie",        "Veggie",            1, 2},
  {"clock",         "Clock",             2, 0},
  {"timer",         "Timer",             2, 1},
  {"soften_melt",   "Soften/Melt",       2, 2},
  {"power",         "Power",             3, 0},
  {"cook_time",     "Cook Time",         3, 1},
  {"defrost",       "Defrost",           3, 2},
  {"1",             "1",                 4, 0},
  {"2",             "2",                 4, 1},
  {"3",             "3",                 4, 2},
  {"4",             "4",                 5, 0},
  {"5",             "5",                 5, 1},
  {"6",             "6",                 5, 2},
  {"7",             "7",                 6, 0},
  {"8",             "8",                 6, 1},
  {"9",             "9",                 6, 2},
  {"stop",          "Stop/Clear",        7, 0},
  {"clear",         "Stop/Clear",        7, 0},
  {"0",             "0",                 7, 1},
  {"start",         "Start/+30 Sec",     7, 2},
  {"start_plus30",  "Start/+30 Sec",     7, 2},
  {"start_30s",     "Start/+30 Sec",     7, 2},
};

static const size_t kKeyCount = sizeof(kKeyMap) / sizeof(kKeyMap[0]);

// Serial command settings.
static constexpr unsigned long kDefaultBaudRate = 115200;
static constexpr unsigned long kDefaultPulseMs = 150;
static constexpr unsigned long kSerialTimeoutMs = 25;  // For command parsing

// Holds the currently active key press state.
struct ActivePress {
  int16_t keyIndex = -1;               // Index into kKeyMap, -1 if idle
  unsigned long releaseDeadline = 0;   // 0 = hold until explicit release
};

static ActivePress g_activePress;
static String g_commandBuffer;

// Forward declarations
void processCommand(const String &line);
void printHelp();
void listKeys();
int16_t findKeyIndex(const String &command);
void startKeyPress(int16_t keyIndex, unsigned long holdMs);
void maintainActivePress();
void releaseActivePress();
void setColumnIdle(uint8_t columnIndex);
void setAllIdle();

void setup() {
  Serial.begin(kDefaultBaudRate);
  Serial.setTimeout(kSerialTimeoutMs);

  // Put every pin into a known high-impedance state to match the passive
  // behaviour of the original keypad.
  setAllIdle();

  Serial.println(F("MD1001LB microwave keypad controller"));
  Serial.println(F("Type 'help' for a list of commands."));
  Serial.println();
}

void loop() {
  // Serial command parsing (simple line based parser)
  while (Serial.available() > 0) {
    char c = static_cast<char>(Serial.read());
    if (c == '\r') {
      continue;  // ignore carriage return
    }
    if (c == '\n') {
      if (g_commandBuffer.length() > 0) {
        processCommand(g_commandBuffer);
        g_commandBuffer = String();
      }
    } else {
      g_commandBuffer += c;
      // Prevent runaway buffers if a host forgets to send a newline.
      if (g_commandBuffer.length() > 80) {
        g_commandBuffer = String();
        Serial.println(F("ERR: command too long"));
      }
    }
  }

  // Maintain any active key presses.
  maintainActivePress();
}

void processCommand(const String &line) {
  String trimmed = line;
  trimmed.trim();
  if (trimmed.length() == 0) {
    return;
  }

  // Tokenise (supports at most three arguments)
  String tokens[4];
  uint8_t count = 0;
  int start = 0;
  while (count < 4) {
    int spaceIdx = trimmed.indexOf(' ', start);
    if (spaceIdx < 0) {
      tokens[count++] = trimmed.substring(start);
      break;
    }
    tokens[count++] = trimmed.substring(start, spaceIdx);
    start = spaceIdx + 1;
    while (start < trimmed.length() && trimmed[start] == ' ') {
      ++start;
    }
    if (start >= trimmed.length()) {
      break;
    }
  }

  for (uint8_t i = 0; i < count; ++i) {
    tokens[i].toLowerCase();
  }

  const String &cmd = tokens[0];
  if (cmd == F("help")) {
    printHelp();
  } else if (cmd == F("list")) {
    listKeys();
  } else if (cmd == F("press") || cmd == F("pulse")) {
    if (count < 2) {
      Serial.println(F("ERR: press <key> [duration_ms]"));
      return;
    }
    int16_t index = findKeyIndex(tokens[1]);
    if (index < 0) {
      Serial.println(F("ERR: unknown key"));
      return;
    }
    unsigned long duration = kDefaultPulseMs;
    if (count >= 3) {
      duration = tokens[2].toInt();
      if (duration == 0) {
        duration = kDefaultPulseMs;
      }
    }
    startKeyPress(index, duration);
  } else if (cmd == F("hold")) {
    if (count < 2) {
      Serial.println(F("ERR: hold <key>"));
      return;
    }
    int16_t index = findKeyIndex(tokens[1]);
    if (index < 0) {
      Serial.println(F("ERR: unknown key"));
      return;
    }
    startKeyPress(index, 0);  // 0 => indefinite hold
  } else if (cmd == F("release")) {
    if (g_activePress.keyIndex < 0) {
      Serial.println(F("OK: nothing to release"));
    } else {
      releaseActivePress();
      Serial.println(F("OK"));
    }
  } else if (cmd == F("status")) {
    if (g_activePress.keyIndex < 0) {
      Serial.println(F("Status: idle"));
    } else {
      Serial.print(F("Status: holding "));
      Serial.print(kKeyMap[g_activePress.keyIndex].label);
      if (g_activePress.releaseDeadline == 0) {
        Serial.println(F(" (until release)"));
      } else {
        Serial.print(F(" ("));
        Serial.print((long)(g_activePress.releaseDeadline - millis()));
        Serial.println(F(" ms remaining)"));
      }
    }
  } else {
    Serial.print(F("ERR: unknown command '"));
    Serial.print(cmd);
    Serial.println(F("'"));
  }
}

void printHelp() {
  Serial.println(F("Available commands:"));
  Serial.println(F("  help                Show this help text"));
  Serial.println(F("  list                List all valid key names"));
  Serial.println(F("  press <key> [ms]    Tap the key for N milliseconds"));
  Serial.println(F("  pulse <key> [ms]    Alias of 'press'"));
  Serial.println(F("  hold <key>          Hold the key until 'release'"));
  Serial.println(F("  release             Release the currently held key"));
  Serial.println(F("  status              Print the active key state"));
  Serial.println();
  Serial.println(F("Examples:"));
  Serial.println(F("  press start"));
  Serial.println(F("  press 1 100"));
  Serial.println(F("  hold cook_time"));
}

void listKeys() {
  Serial.println(F("Known key commands:"));
  for (size_t i = 0; i < kKeyCount; ++i) {
    Serial.print(F("  "));
    Serial.print(kKeyMap[i].command);
    Serial.print(F("  ("));
    Serial.print(kKeyMap[i].label);
    Serial.println(F(")"));
  }
}

int16_t findKeyIndex(const String &command) {
  for (size_t i = 0; i < kKeyCount; ++i) {
    if (command.equalsIgnoreCase(kKeyMap[i].command)) {
      return static_cast<int16_t>(i);
    }
  }
  return -1;
}

void startKeyPress(int16_t keyIndex, unsigned long holdMs) {
  if (keyIndex < 0 || keyIndex >= static_cast<int16_t>(kKeyCount)) {
    Serial.println(F("ERR: invalid key index"));
    return;
  }

  releaseActivePress();

  g_activePress.keyIndex = keyIndex;
  g_activePress.releaseDeadline = (holdMs == 0) ? 0 : millis() + holdMs;

  uint8_t rowIndex = kKeyMap[keyIndex].row;
  uint8_t columnIndex = kKeyMap[keyIndex].column;

  if (rowIndex >= kRowCount || columnIndex >= kColumnCount) {
    Serial.println(F("ERR: key mapping out of range"));
    g_activePress.keyIndex = -1;
    g_activePress.releaseDeadline = 0;
    return;
  }

  // Ensure row stays high impedance.
  pinMode(kRowPins[rowIndex], INPUT);
  // Prepare the column for driving.
  digitalWrite(kColumnPins[columnIndex], LOW);
  pinMode(kColumnPins[columnIndex], OUTPUT);

  // Immediate sync so the first scan already sees the key.
  int state = digitalRead(kRowPins[rowIndex]);
  digitalWrite(kColumnPins[columnIndex], state);

  Serial.print(F("OK: pressing "));
  Serial.println(kKeyMap[keyIndex].label);
}

void maintainActivePress() {
  if (g_activePress.keyIndex < 0) {
    return;
  }

  uint8_t rowIndex = kKeyMap[g_activePress.keyIndex].row;
  uint8_t columnIndex = kKeyMap[g_activePress.keyIndex].column;

  if (rowIndex >= kRowCount || columnIndex >= kColumnCount) {
    releaseActivePress();
    Serial.println(F("ERR: active key mapping out of range"));
    return;
  }

  int state = digitalRead(kRowPins[rowIndex]);
  digitalWrite(kColumnPins[columnIndex], state);

  if (g_activePress.releaseDeadline != 0 && millis() >= g_activePress.releaseDeadline) {
    releaseActivePress();
    Serial.println(F("OK"));
  }
}

void releaseActivePress() {
  if (g_activePress.keyIndex < 0) {
    return;
  }

  uint8_t columnIndex = kKeyMap[g_activePress.keyIndex].column;
  setColumnIdle(columnIndex);

  g_activePress.keyIndex = -1;
  g_activePress.releaseDeadline = 0;
}

void setColumnIdle(uint8_t columnIndex) {
  if (columnIndex >= kColumnCount) {
    return;
  }
  digitalWrite(kColumnPins[columnIndex], LOW);
  pinMode(kColumnPins[columnIndex], INPUT);
}

void setAllIdle() {
  for (size_t i = 0; i < kRowCount; ++i) {
    pinMode(kRowPins[i], INPUT);
  }
  for (size_t i = 0; i < kColumnCount; ++i) {
    setColumnIdle(i);
  }
}
