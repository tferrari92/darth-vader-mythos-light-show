---
name: upload-to-arduino
description: Compile and upload the darth-vader-mythos-light-show master sketch to the Arduino UNO. Use whenever the user wants to build, flash, or upload the light-show firmware to the board, or edits master/master.ino and wants it on the hardware.
---

# Upload the light show to the Arduino

The master sketch lives in `master/master.ino` (Arduino UNO, `arduino:avr:uno`).

## Key gotcha: the MultiRGBWLeds library

`arduino-cli` will NOT find `MultiRGBWLeds.h` on its own — the correct version is
**not** in `~/Documents/Arduino/libraries/` (that folder + `sketch_feb17a` hold an
OLD, incompatible copy whose `LampColor`/`LampPosition` enums are missing values
like `Red` and `BackRight`, which causes compile errors).

The **correct** library ships inside this repo's parent tree:

```
/Users/tomas/Desktop/i-did/publish/MultiRGBWLeds
```

Always pass it explicitly with `--library`.

## Detect the port

```bash
arduino-cli board list
```

Look for the row whose Board Name is `Arduino UNO` (typically
`/dev/cu.usbmodem21301`, but confirm each time).

## Compile + upload in one step

Run from the sketch's project dir. `--library` is only valid on `compile`, so use
`compile --upload` (the plain `upload` subcommand rejects `--library`):

```bash
cd /Users/tomas/Desktop/i-did/publish/darth-vader-mythos-light-show
arduino-cli compile \
  --fqbn arduino:avr:uno \
  --library /Users/tomas/Desktop/i-did/publish/MultiRGBWLeds \
  --upload -p /dev/cu.usbmodem21301 \
  master
```

Success looks like `Sketch uses NNNNN bytes ...`; add `-v` and grep for
`Avrdude done.  Thank you.` to confirm the flash write.

## Notes

- Other libraries (DYPlayer/dyplayer, IRremote 3.x, Adafruit PWM Servo Driver,
  Adafruit BusIO) resolve automatically from the installed sketchbook libraries.
  Keep **IRremote 3.x** — 4.x compiles but silently fails to decode the IR buttons.
- Lamp brightness scale is **0–10** (10 = max). The resting/ambient scene is set in
  `setAmbient()` and `fadeToAmbient()` in `master/master.ino`.
