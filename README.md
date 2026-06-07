# Darth Vader Mythos Light Show

An IR-remote-driven light-and-effects show for a Darth Vader Mythos statue, built on an
Arduino Uno. Each track on the remote triggers a choreographed routine: RGBW lamp
animations, fog (atomizer), the lightsaber, a fan, and a NeoPixel fire/blue effect — all
synchronized to music played from a DFPlayer Mini.

## Architecture

The system is split across two microcontrollers communicating over I²C:

- **Master — Arduino Uno** ([`master/master.ino`](master/master.ino))
  - Drives **4× RGBW lamps** through a **PCA9685** 16-channel PWM driver, using the
    [MultiRGBWLeds](https://github.com/tferrari92/MultiRGBWLeds) library for the animations
    (crossfade, set, side-to-side, spin, flash).
  - Plays audio from a **DFPlayer Mini** MP3 module (SoftwareSerial).
  - Reads an **IR remote** to select tracks.
  - Switches a **fog/atomizer relay**, a **lightsaber relay**, and a **fan** (H-bridge).
  - Sends I²C commands to the ATtiny85 slave: `0xAAAA` → fire animation, `0xBBBB` → blue animation.

- **Slave — ATtiny85** ([`attiny85/attiny85.ino`](attiny85/attiny85.ino))
  - Drives a **55-LED WS2812B NeoPixel strip** with FastLED, running a Perlin-noise fire
    effect or a solid blue fill on command. Offloaded to its own MCU so the blocking LED
    timing doesn't disrupt the master's choreography.
  - [`FastLED_RGBW.h`](attiny85/FastLED_RGBW.h) adds RGBW support to FastLED
    (credit: Jim Bumgardner & David Madison).

## Tracks

The IR remote maps five tracks plus stop, each with its own routine (timed to the audio):

1. **I Did – Original** — orange/red crossfade + fan + fogger + saber + fire
2. **I Did – Cumbia** — green/indigo side-to-side and spin choreography
3. **I Did – Lofi** — indigo/magenta static set
4. **I Did – Metal** — orange/red static set + fire
5. **For Whom The Bell Tolls** — delayed red fade-in
- **Play/Pause** — stop

## Dependencies

Install via the Arduino IDE Library Manager (or "Add .ZIP Library"):

- **[MultiRGBWLeds](https://github.com/tferrari92/MultiRGBWLeds)** — companion library for this project (RGBW lamp animations)
- **Adafruit PWM Servo Driver Library** — PCA9685 control
- **DYPlayerArduino** — DFPlayer Mini MP3 module
- **IRremote**
- **FastLED** — for the ATtiny85 sketch

`SoftwareSerial` and `Wire` ship with the Arduino core.

## Build & flash

1. Install the dependencies above.
2. **Master:** open [`master/master.ino`](master/master.ino), select **Arduino Uno**, upload.
3. **Slave:** open [`attiny85/attiny85.ino`](attiny85/attiny85.ino), select the ATtiny85 via
   [ATTinyCore](https://github.com/SpenceKonde/ATTinyCore), upload.

> **Audio:** the MP3 tracks live on the DFPlayer Mini's microSD card and are **not** included
> in this repository.

## Notes

Wiring diagrams, a bill of materials, and the audio are intentionally left out of this repo
for now — this is the firmware only.

## License

[MIT](LICENSE) © Tomas Ferrari
