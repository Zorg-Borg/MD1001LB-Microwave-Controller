# MD1001LB Microwave Keypad Controller

This project contains an Arduino sketch that replaces the membrane keypad on a
Midea MD1001LB microwave control PCB. The Arduino mirrors button presses across
the keypad matrix so the original controller sees the exact same electrical
signals that would be produced by the stock keypad.

## Hardware assumptions

* The keypad is wired as an 8×3 matrix. Eight row lines are scanned by the
  microwave control PCB, while three column lines are read as inputs.
* The row lines from the PCB are connected to Arduino digital pins **2–9**.
  These pins always remain in high-impedance mode so the microwave can continue
  driving the scan signals.
* The column lines from the PCB are connected to Arduino digital pins
  **10–12** (from left to right they land on D11, D10, and D12 respectively).
  When a key is simulated these pins are actively driven to the same logic
  level that appears on the matching row line, emulating a closed switch.

If your PCB revision wires the keypad differently simply update the `kRowPins`
and `kColumnPins` arrays inside `MD1001LB_Controller.ino`.

## Flashing the firmware

1. Open `MD1001LB_Controller.ino` in the Arduino IDE (or your preferred
   toolchain).
2. Select the correct board/port for your Arduino controller.
3. Upload the sketch.

After flashing, open the serial monitor at **115200 baud** (newline terminated)
to interact with the CLI.

## Serial command reference

```
help
    Show the command list.

list
    Display every available key command.

press <key> [duration_ms]
    Tap the specified key for the provided duration (default 150 ms).

pulse <key> [duration_ms]
    Alias for `press`.

hold <key>
    Hold the key until a `release` command is received.

release
    Release the currently held key (if any).

status
    Print the current key press state.
```

Key names are lowercase tokens such as `start`, `stop`, `cook_time`, `2`, and so
on. Run `list` to see every supported alias along with the human-readable label
for each microwave button.

## Safety notes

* Disconnect mains power from the microwave before modifying any wiring.
* Keep the Arduino and any prototyping boards well isolated from the high
  voltage sections of the microwave.
* The Arduino simulates a passive keypad. Do not reconfigure the pins to drive
  against the control PCB, otherwise you risk damaging the microwave controller.
