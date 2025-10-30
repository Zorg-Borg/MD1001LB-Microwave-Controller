# MD1001LB Button Testing Checklist

This document provides a systematic approach to testing all buttons on the MD1001LB microwave controller to verify correct keypad matrix mappings.

## Testing Instructions

For each button below:
1. Send the serial command (e.g., `press start`)
2. Observe which physical button activates on the microwave
3. Mark ✅ if correct, ❌ if wrong, and note which button actually activated

## Current Button Status

### Control Buttons
- [ ] **STOP/CLEAR** (`press stop` or `press clear`)
  - Expected: Stop/Clear button
  - Current mapping: Row 7, Column 2 (swapped with START to fix issue)
  - Actual button activated: _____________
  - Status: 🔧 FIXED - swapped positions with START

- [ ] **START/+30 SEC** (`press start` or `press start_plus30`)
  - Expected: Start/+30 Sec button
  - Current mapping: Row 7, Column 0 (swapped with STOP to fix issue)
  - Actual button activated: _____________
  - Status: 🔧 FIXED - swapped positions with STOP

### Number Pad (0-9)
- [ ] **0** (`press 0`)
  - Expected: 0 button
  - Current mapping: Row 7, Column 1
  - Actual button activated: _____________

- [ ] **1** (`press 1`)
  - Expected: 1 button
  - Current mapping: Row 5, Column 1
  - Actual button activated: _____________

- [ ] **2** (`press 2`)
  - Expected: 2 button
  - Current mapping: Row 4, Column 1
  - Actual button activated: _____________

- [ ] **3** (`press 3`)
  - Expected: 3 button
  - Current mapping: Row 3, Column 1
  - Actual button activated: _____________

- [ ] **4** (`press 4`)
  - Expected: 4 button
  - Current mapping: Row 2, Column 1
  - Actual button activated: _____________

- [ ] **5** (`press 5`)
  - Expected: 5 button
  - Current mapping: Row 1, Column 1
  - Actual button activated: _____________

- [ ] **6** (`press 6`)
  - Expected: 6 button
  - Current mapping: Row 0, Column 1
  - Actual button activated: _____________

- [ ] **7** (`press 7` or `press soften_melt`)
  - Expected: 7/Soften/Melt button (shared pad)
  - Current mapping: Row 6, Column 0
  - Actual button activated: _____________

- [ ] **8** (`press 8`)
  - Expected: 8 button
  - Current mapping: Row 6, Column 1
  - Actual button activated: _____________

- [ ] **9** (`press 9` or `press reheat`)
  - Expected: 9/Reheat button (shared pad)
  - Current mapping: Row 6, Column 2
  - Actual button activated: _____________

### Function Buttons (Left Column)
- [ ] **CLOCK** (`press clock`)
  - Expected: Clock button
  - Current mapping: Row 0, Column 0
  - Actual button activated: _____________

- [ ] **DEFROST** (`press defrost`)
  - Expected: Defrost button
  - Current mapping: Row 1, Column 0
  - Actual button activated: _____________

- [ ] **TIMER** (`press timer`)
  - Expected: Timer button
  - Current mapping: Row 2, Column 0
  - Actual button activated: _____________

- [ ] **POWER** (`press power`)
  - Expected: Power button
  - Current mapping: Row 3, Column 0
  - Actual button activated: _____________

- [ ] **COOK TIME** (`press cook_time`)
  - Expected: Cook Time button
  - Current mapping: Row 4, Column 0
  - Actual button activated: _____________

### Quick-Cook Presets (Right Column)
- [ ] **AUTO COOK** (`press auto_cook`)
  - Expected: Auto Cook preset (top right)
  - Current mapping: Row 0, Column 2
  - Actual button activated: _____________

- [ ] **VEGGIE** (`press veggie`)
  - Expected: Veggie preset
  - Current mapping: Row 1, Column 2
  - Actual button activated: _____________

- [ ] **RICE** (`press rice`)
  - Expected: Rice preset
  - Current mapping: Row 2, Column 2
  - Actual button activated: _____________

- [ ] **POTATO** (`press potato`)
  - Expected: Potato preset
  - Current mapping: Row 3, Column 2
  - Actual button activated: _____________

- [ ] **FROZEN ENTRÉE** (`press frozen_entree`)
  - Expected: Frozen Entrée preset
  - Current mapping: Row 4, Column 2
  - Actual button activated: _____________

- [ ] **FROZEN PIZZA** (`press frozen_pizza`)
  - Expected: Frozen Pizza preset
  - Current mapping: Row 5, Column 2
  - Actual button activated: _____________

## Testing Notes

### Known Issues
1. **START button**: Fixed - swapped from Row 7, Column 2 to Row 7, Column 0
2. **STOP button**: Fixed - swapped from Row 7, Column 0 to Row 7, Column 2

Note: These buttons were initially mapped incorrectly. They have been swapped based on typical microwave keypad layouts where START is usually on the left and STOP/CLEAR is on the right of the bottom row.

### Additional Observations
Please note any observations here:
- Do any buttons trigger multiple functions?
- Are there any buttons that don't respond at all?
- Are there any unexpected behaviors?

## Matrix Layout Reference

Current column pin mapping: `{11, 10, 12}` = {Left, Center, Right}

```
Row/Col | Col 0 (Pin 11) | Col 1 (Pin 10) | Col 2 (Pin 12)
--------|----------------|----------------|----------------
Row 0   | CLOCK          | 6              | AUTO COOK
Row 1   | DEFROST        | 5              | VEGGIE
Row 2   | TIMER          | 4              | RICE
Row 3   | POWER          | 3              | POTATO
Row 4   | COOK TIME      | 2              | FROZEN ENTRÉE
Row 5   | (empty)        | 1              | FROZEN PIZZA
Row 6   | 7/SOFTEN/MELT  | 8              | 9/REHEAT
Row 7   | START/+30SEC   | 0              | STOP/CLEAR
```

## Debugging Tips

1. **Test systematically**: Start with numbers 0-9, then function buttons, then presets
2. **Document wrong buttons**: If pressing "1" activates "2", note that clearly
3. **Look for patterns**: If buttons are consistently off by one row/column, that indicates a systematic issue
4. **Test edge cases**: Pay special attention to shared number/preset buttons
5. **Serial monitor**: Use 115200 baud, newline termination in Arduino Serial Monitor

## Reporting Results

After testing, please provide:
1. Which buttons work correctly (✅)
2. Which buttons activate wrong functions (❌ + which button actually pressed)
3. Any patterns you notice in the incorrect mappings
4. Photos/videos of physical button layout if possible
