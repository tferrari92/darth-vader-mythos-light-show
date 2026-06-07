# Darth Vader Mythos Light Show

An IR-remote-driven light-and-effects show for a Darth Vader Mythos statue, built on an
Arduino Uno. Each track on the remote triggers a choreographed routine synchronized to
music played from a DFPlayer Mini.

The **basic build is 4 RGBW lamps + audio** — that's all you need to run the show. Three
extra effects are **optional** and toggled with feature flags (see
[Configuration](#configuration)): a NeoPixel front strip, smoke (humidifier + fan), and the
lightsaber. With every flag off, the sketch runs the lamps and audio standalone — it does
**not** wait for the ATtiny85 or touch any relays.

## Architecture

- **Master — Arduino Uno** ([`master/master.ino`](master/master.ino))
  - Drives **4× RGBW lamps** through a **PCA9685** 16-channel PWM driver, using the
    [MultiRGBWLeds](https://github.com/tferrari92/MultiRGBWLeds) library for the animations
    (crossfade, set, side-to-side, spin, flash). *(always on)*
  - Plays audio from a **DFPlayer Mini** MP3 module (SoftwareSerial). *(always on)*
  - Reads an **IR remote** to select tracks. *(always on)*
  - *Optional:* switches a **smoke** rig (humidifier relay + fan H-bridge) and a
    **lightsaber relay**.
  - *Optional:* sends I²C commands to the ATtiny85 front-strip slave:
    `0xAAAA` → fire animation, `0xBBBB` → blue animation.

- **Slave — ATtiny85** ([`attiny85/attiny85.ino`](attiny85/attiny85.ino)) *(optional — front strip)*
  - Drives a **55-LED WS2812B NeoPixel strip** with FastLED, running a Perlin-noise fire
    effect or a solid blue fill on command. Offloaded to its own MCU so the blocking LED
    timing doesn't disrupt the master's choreography.
  - [`FastLED_RGBW.h`](attiny85/FastLED_RGBW.h) adds RGBW support to FastLED
    (credit: Jim Bumgardner & David Madison).
  - Only needed if you enable the front strip; otherwise you can ignore the `attiny85/` sketch.

## Configuration

Optional hardware is controlled by feature flags at the top of
[`master/master.ino`](master/master.ino). Arduino has no runtime env vars, so these are
compile-time toggles. They default to **off**, so the basic 4-lamp + audio build works
out of the box.

```cpp
#define ENABLE_FRONT_STRIP 0   // ATtiny85 NeoPixel front strip (I2C slave)
#define ENABLE_SMOKE       0   // Humidifier + fan
#define ENABLE_LIGHTSABER  0   // Lightsaber relay
```

Set a flag to `1` to enable that effect. The flags are wrapped in `#ifndef`, so you can also
override them at build time **without editing the file** (the closest thing to an "env var"):

```bash
# arduino-cli
arduino-cli compile --fqbn arduino:avr:uno master \
  --build-property "build.extra_flags=-DENABLE_SMOKE=1 -DENABLE_LIGHTSABER=1"
```

```ini
; platformio.ini
build_flags = -DENABLE_FRONT_STRIP=1 -DENABLE_SMOKE=1 -DENABLE_LIGHTSABER=1
```

Note: the lamp choreography is timed to the audio, so the small settle delays around the
front-strip commands are preserved even when the strip is disabled — the show stays in sync
in every configuration.

## Tracks

The IR remote maps five tracks plus stop, each with its own routine (timed to the audio).
Effects in *italics* below only fire if their feature flag is enabled:

1. **I Did – Original** — orange/red crossfade *(+ smoke, saber, front-strip fire)*
2. **I Did – Cumbia** — green/indigo side-to-side and spin choreography *(+ smoke, saber, front-strip blue)*
3. **I Did – Lofi** — indigo/magenta static set *(+ front-strip blue)*
4. **I Did – Metal** — orange/red static set *(+ front-strip fire)*
5. **For Whom The Bell Tolls** — delayed red fade-in
- **Play/Pause** — stop

## Dependencies

Install via the Arduino IDE Library Manager (or "Add .ZIP Library"):

**Master (always required):**
- **[MultiRGBWLeds](https://github.com/tferrari92/MultiRGBWLeds)** — companion library for this project (RGBW lamp animations)
- **Adafruit PWM Servo Driver Library** — PCA9685 control
- **DYPlayerArduino** — DFPlayer Mini MP3 module
- **IRremote**

**ATtiny85 front strip (only if `ENABLE_FRONT_STRIP`):**
- **FastLED**

`SoftwareSerial` and `Wire` ship with the Arduino core.

## Build & flash

1. Install the dependencies above.
2. (Optional) Set the [feature flags](#configuration) for any extra hardware you wired up.
3. **Master:** open [`master/master.ino`](master/master.ino), select **Arduino Uno**, upload.
4. **Slave (only if you enabled the front strip):** open
   [`attiny85/attiny85.ino`](attiny85/attiny85.ino), select the ATtiny85 via
   [ATTinyCore](https://github.com/SpenceKonde/ATTinyCore), upload.

> **Audio:** the MP3 tracks live on the DFPlayer Mini's microSD card and are **not** included
> in this repository.

## Notes

Wiring diagrams, a bill of materials, and the audio are intentionally left out of this repo
for now — this is the firmware only.

## License

[MIT](LICENSE) © Tomas Ferrari
