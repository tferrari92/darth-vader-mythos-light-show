# Darth Vader Mythos Light Show

An IR-remote-driven light-and-effects show for a Darth Vader Mythos statue, built on an
Arduino Uno. Each track on the remote triggers a choreographed routine synchronized to
music played from a DFPlayer Mini.

The **basic build is 4 RGBW lamps + audio** — that's all you need to run the show. Three
extra effects are **optional** and toggled with feature flags (see
[Configuration](#configuration)): a NeoPixel front strip, smoke (humidifier + fan), and the
lightsaber. With every flag off, the sketch runs the lamps and audio standalone — it does
**not** wait for the ATtiny85 or touch any relays.

## Demo

- 🎥 **Video demo:** _TODO: YouTube link_
- 🖼️ **Build photos:** _TODO: link_

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

## Bill of materials

Build the **Basic** set first; add any of the optional modules later (each maps to a
[feature flag](#configuration)).

### Basic — 4 lamps + audio
- [Arduino Uno (or compatible)](https://a.co/d/03DOTip2)
- [PCA9685 16-channel PWM driver](https://a.co/d/0jiwvAFP)
- [4× RGBW LED lamps (common-anode)](https://a.co/d/08rguR4Z)
- [DY-SV5W Playback Module MP3 Board](https://a.co/d/0iwnKNiH) 
- [3.5mm Aux Cable](https://a.co/d/05ZY8Oyp)
- [Speaker (I used a Bose SoundLink Mini I)](https://a.co/d/0ehsrhri)
- [microSD card](https://a.co/d/0cLakK3m)
- [IR receiver + IR remote](https://a.co/d/0esAqea8)
- [12V powe supply](https://a.co/d/0cVwCTjt)
- [5V power supply](https://a.co/d/0ai2ccZ6)

#### Optional / Quality of life Items
  - [Lever Wire Connectors](https://a.co/d/073DuDCG)
  - [4x 5 Pin Magnetic Pogo Pin Connector](https://a.co/d/08rJCHPg)
  - [12V DC Male + Female Connector](https://a.co/d/08ywx2uR)
  - [USB Extensions Cable](https://a.co/d/0bbNSKzB)
  - [Matte Window Film](https://a.co/d/0bbNSKzB)

### Add-on — Front LED strip
- [Digispark ATtiny85](https://a.co/d/04rXa1Af)
- [3.28ft/1m 60 Pixel RGBW (Warm White) Led Strip](https://a.aliexpress.com/_m0XdlDt)

### Add-on — Back smoke (LED + fan + humidifier)
- [5V Ultrasonic mist / humidifier module](https://a.aliexpress.com/_m0UEqIB)
- [5V L9110 Fan Motor Module](https://a.aliexpress.com/_msmvBKT) 
- LED — _TODO: AliExpress link_
- Relay module — _TODO: AliExpress link_

### Add-on — Lightsaber
- [3V 260mm White Led Filament](https://a.aliexpress.com/_mP3VffN)
- Transparent Acrylic Tube
- UV Resin
- Relay module — _TODO: AliExpress link_

## Wiring

<p title="" align="center"> <img src="https://i.imgur.com/VOA90VD.png"> </p>

The diagram shows the full wiring including all optional modules — wire only the sections for the modules you're building.

## Dependencies

Install via the Arduino IDE Library Manager (or "Add .ZIP Library"):

**Master (always required):**
- **[MultiRGBWLeds](https://github.com/tferrari92/MultiRGBWLeds)** — companion library for this project (RGBW lamp animations)
- **Adafruit PWM Servo Driver Library** — PCA9685 control
- **DYPlayerArduino** — DFPlayer Mini MP3 module
- **IRremote — version 3.x** ⚠️

> **Important: use IRremote 3.x, not 4.x.** This sketch uses IRremote's 3.x
> receive API. IRremote 4.x still *compiles* (via a deprecated compatibility
> shim) but **silently fails to decode** — buttons do nothing. In the Library
> Manager, pick a 3.x version (e.g. 3.9.0).

**ATtiny85 front strip (only if `ENABLE_FRONT_STRIP`):**
- **FastLED**

`SoftwareSerial` and `Wire` ship with the Arduino core.

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

## Build & flash

1. Install the dependencies above.
2. (Optional) Set the [feature flags](#configuration) for any extra hardware you wired up.
3. **Master:** open [`master/master.ino`](master/master.ino), select **Arduino Uno**, upload.
4. **Slave (only if you enabled the front strip):** open
   [`attiny85/attiny85.ino`](attiny85/attiny85.ino), select the ATtiny85 via
   [ATTinyCore](https://github.com/SpenceKonde/ATTinyCore), upload.

## Controls

The IR remote maps each track to a routine (timed to the audio). Effects in *italics*
only fire if their feature flag is enabled. (Button codes are for the bundled NEC remote;
if yours differs, read the HEX over Serial at 9600 and update the cases in `master.ino`.)

| Button | Action |
|--------|--------|
| **1** | **I Did – Original** |
| **2** | **I Did – Cumbia** |
| **3** | **I Did – Lofi** |
| **4** | **I Did – Metal**  |
| **5** | **For Whom The Bell Tolls** |
| **Power** | Toggle the always-on ambient scene |

## Enclosure

3D-printable enclosure that holds everything together — STL files live in
[`enclosure/`](enclosure/). _(TODO: add STL files.)_

## Audio

The five music tracks live on the DFPlayer Mini's **microSD card** as `0001.mp3` … `0006.mp3`
(track 6 is a short silence used as "stop"). They are **not** committed to this repo.

- ⬇️ [**Download tracks**](https://drive.google.com/drive/folders/14aJys-XmJGN54O0cnzooF6nU0tt411FF?usp=sharing)

## License

[MIT](LICENSE) © Tomas Ferrari
