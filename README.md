# ESP8266 4-Digit Nixie Clock — Firmware

Firmware for a 4-digit Nixie/VFD clock built around a **Wemos D1 Mini (ESP8266)**.
The clock keeps time from NTP with fallback to an optional battery-backed DS1307 RTC,
drives four tubes through a single 8-bit shift register, and is configured either from
the one and only front-panel button or from a built-in web interface.

Current software version: `8` (`SOFTWARE_VERSION` in [`Software/ClockDefs.h`](Software/ClockDefs.h))

See [`USER_MANUAL.md`](USER_MANUAL.md) for end-user documentation — first-time setup, the
button menu, the web interface and troubleshooting.

## Repository layout

| Directory | Description |
|---|---|
| [`Software`](Software) | The Arduino sketch and all firmware sources. This is what the rest of this README describes. |
| `Hardware` | KiCad schematic, PCB and footprint libraries for the main board. Not covered here. |
| [`platformio.ini`](platformio.ini) | The build configuration — board settings, pinned library versions and the upload targets. |

## Firmware highlights

- **Software-timed multiplex with per-digit phases** — a single `timer1` ISR walks the four
  digits, and each digit slot is split into up to three phases (show previous value, switch to
  new value, blank). Dimming and crossfading fall out of the phase boundaries rather than
  needing extra hardware or PWM channels.
- **Crossfade and scrollback from the same mechanism** — "fade" slides the phase-1 switch point
  across the digit's on-time; "scrollback" reuses the same switch to step a digit down through
  9…0 when it rolls over.
- **Non-blocking NTP** — time is fetched over `ESPAsyncUDP`, so the ~78 Hz display refresh and
  the web server are never stalled by a network round trip. DST is handled by the C library
  from a POSIX TZ string, so no DST rules live in the firmware.
- **Three-tier time source** — NTP when it is fresh, the DS1307 RTC when it is not, and the
  ESP8266's internal clock as a last resort. The RTC is written on every NTP update and read
  back once a minute, so the clock survives both reboots and internet outages.
- **Adaptive brightness** — the LDR reading is smoothed with an exponential average over a
  configurable number of samples, then scaled by threshold/sensitivity into the 1…20 count
  on-time used by the multiplex, with a configurable floor so the tubes never go fully dark.
- **Perceptual LED dimming** — WS2812B backlights are driven through a non-linear brightness
  curve (`dim_curve`) plus a 16-step channel curve, so the colour channels track what the eye
  sees rather than the raw PWM value.
- **Whole configuration in one JSON file** — every setting lives in a single struct that is
  serialised to `/config.json` in SPIFFS; running statistics go to `/stats.json`. There is no
  EEPROM layout to version.
- **Full configuration twice over** — every setting is reachable both from the single button
  (36 menu positions, with irrelevant entries automatically skipped) and from the web UI.

## Source layout

All sources live in [`Software`](Software), which is the PlatformIO `src_dir`. The main
translation unit is `main.cpp`; the rest are compiled alongside it.

| File | Responsibility |
|---|---|
| `main.cpp` | Display ISR and multiplex scheduling, `setup()`/`loop()`, the button menu state machine, blanking logic, LDR dimming, RTC access, and all web page handlers. |
| `ClockDefs.h` | Pin assignments, software version, and every configuration constant: mode numbers, limits and defaults. |
| `DisplayDefs.h` | Per-digit display types (`BLANKED`/`DIMMED`/`NORMAL`/`FADE`/`SCROLL`/`BLINK`/`BRIGHT`), separator LED modes, status LED modes, and the cathode/anode bit mapping for the shift register. |
| `Globals.h` | Component instances and the shared mutable state (display buffers, blanking flags, LDR state, usage counters). |
| `ClockButton.{h,cpp}` | Debounce plus press-length classification (momentary, >1 s, >2 s, >8 s), each with a separate "pressed" and "pressed and released" state. |
| `Transition.{h,cpp}` | The "slots" animation — scroll in, hold, scramble out — used to show the date once a minute. |
| `LEDManager.{h,cpp}` | WS2812B backlight strategies (fixed, cycle, colour-time, day-of-week), pulse and dim handling, brightness curves, and the boot diagnostic LEDs. |
| `NtpAsync.{h,cpp}` | Asynchronous NTP client, TZ string handling, update scheduling and validity tracking. Calls back into the sketch when a new time arrives. |
| `ESP_DS1307.{h,cpp}` | Minimal I²C driver for the DS1307 RTC (BCD conversion, get/set time, run state). |
| `SPIFFS.{h,cpp}` | Load/save of `/config.json` and `/stats.json`, and the `spiffs_config_t` struct that defines the whole configuration. |
| `HtmlServer.{h,cpp}` | Web server instance, HTTP basic auth state, and the HTML fragment builders (nav bar, tables, forms, inputs). |
| `ClockUtils.{h,cpp}` | String/IP/BCD helpers, pin mode setup, and serial debug plumbing. |
| `DebugManager.{h,cpp}` | Serial debug output gate shared by the components. |

## Building and flashing

The project is a [PlatformIO](https://platformio.org/) project. The configuration is in
[`platformio.ini`](platformio.ini) at the repository root, with `src_dir` pointing at
[`Software`](Software), so the firmware and the hardware design share one repository.

Open the repository root in VS Code with the PlatformIO IDE extension, or use the CLI:

```sh
pio run                     # build
pio run -t upload           # build and flash over USB
pio device monitor          # serial console at 115200 baud
pio run -t clean            # discard build output
```

Everything else — the ESP8266 platform, the toolchain and all five libraries — is downloaded
on the first build. Nothing needs installing by hand.

**Board settings** are set in `platformio.ini` and reproduce the Arduino IDE settings the
firmware was developed with:

| Setting | Value | Set by |
|---|---|---|
| Board | LOLIN (Wemos) D1 R2 & mini | `board = d1_mini` |
| CPU frequency | 160 MHz | `board_build.f_cpu` |
| Flash frequency | 80 MHz | `board_build.f_flash` |
| Flash layout | 4M flash, 1M SPIFFS | `board_build.ldscript = eagle.flash.4m1m.ld` |
| lwIP variant | v2 Higher Bandwidth (no features) | `PIO_FRAMEWORK_ARDUINO_LWIP2_HIGHER_BANDWIDTH_LOW_FLASH` |
| Upload speed | 921600 | `upload_speed` |

A SPIFFS partition is required — the configuration is stored there. `pio run -t upload` writes
only the firmware and leaves the filesystem alone, so a re-flash keeps the saved configuration.
There is no `data` directory to upload: `/config.json` and `/stats.json` are written by the
firmware itself at runtime.

**Libraries** are pinned in `platformio.ini` and fetched automatically:

| Library | Version | Source |
|---|---|---|
| Time (`TimeLib`) | `^1.6.1` | `paulstoffregen/Time` |
| NeoPixelBus | `^2.7.3` | `makuna/NeoPixelBus` |
| WiFiManager | `^0.16.0` | `tzapu/WiFiManager` |
| ArduinoJson | `^5.13.2` | `bblanchon/ArduinoJson` |
| ESPAsyncUDP | commit `697c75a0` | `github.com/me-no-dev/ESPAsyncUDP` |

ArduinoJson is deliberately held at 5.x: the 6.x and 7.x APIs are not source compatible and
`Software/SPIFFS.cpp` is written against 5.x. ESPAsyncUDP is unmaintained and absent from the
PlatformIO registry, so it is pinned to the exact commit the firmware was built against rather
than to a moving `master`.

**Over the air** — after the first USB flash, updates can be pushed to a running clock through
the `/update` endpoint it serves:

```sh
CLOCK_HOST=192.168.1.42 pio run -e d1_mini_ota -t upload
```

`CLOCK_HOST` is the clock's IP address, or its mDNS name (shown as "Clock Name" on its web
page) with `.local` appended. This is a plain HTTP form upload to `ESP8266HTTPUpdateServer`,
not the ArduinoOTA protocol, so the `d1_mini_ota` environment uses a `curl` upload command
rather than PlatformIO's `espota`. See
[Over-the-air updates](USER_MANUAL.md#13-over-the-air-firmware-updates) in the user manual for
the equivalent through the web interface.

## Compile-time feature flags

Near the top of `main.cpp`. Append `_OFF` to the name to disable a feature;
`LEDManager.h` carries its own flags for the LED chain layout.

| Flag | Default | Effect |
|---|---|---|
| `FEATURE_FADE` | on | Digit crossfade, and its web/menu settings. |
| `FEATURE_SCROLL` | on | Scrollback effect, and its web/menu settings. |
| `FEATURE_PIR` | on | Motion sensor input and timeout handling. |
| `FEATURE_LED_MODES` | on | Separator LED mode selection in the web UI. |
| `FEATURE_PREHEAT` | off | Digit pre-heat strength option. |
| `FEATURE_STATUS_LEDS` | off | Left/right status LED mode selection. |
| `FEATURE_EXT_LEDS` | off | External/underlight LED dimming option. |
| `DEBUG` | off | Serial debug output (note: the LED data line shares the RX pin). |
| `FEATURE_REVERSE_BACKLIGHTS` | on | Reverse the backlight LED order in the chain. |
| `FEATURE_REVERSE_UNDERLIGHTS` | on | Reverse the underlight LED order in the chain. |
| `FEATURE_UNDERLIGHTS` | off | Compile in the second (underlight) LED chain. |

## How it works

### Display multiplex

`timer1` runs at 5 MHz (`TIM_DIV16`) in single-shot mode and is rearmed by the ISR at the end
of every phase. The time unit is `INT_MUX_COUNTS` = 800 ticks = 160 µs; one digit gets
`COUNTS_PER_DIGIT` = 20 units = 3.2 ms, so a full four-digit frame takes ~12.8 ms (~78 Hz).

Each digit's 20 units are divided into up to three phases, precalculated outside the ISR by
`setDigit()` into `phaseTime[phase][digit]`:

| Phase | Action |
|---|---|
| 0 | Turn the digit on showing the *previous* value (or blank it, or show the final value directly). |
| 1 | Switch the cathodes to the *new* value — the fade/scroll crossover point. |
| 2 | Turn the anode off for the remainder of the slot — this is the dimming gap. |

`dimFactor` (1…20) is how many of the 20 units the digit is lit for; `switchTime` is where
inside that lit period phase 1 happens. Fading therefore costs nothing extra at ISR time: the
switch point simply walks across the slot over `fadeSteps` display passes.

The separator LEDs are serviced from the same ISR: they are turned on at the start of each
frame and off after `separatorDimFactor` digits, which gives them five brightness steps.

### Main loop

`loop()` services the web server and mDNS, polls the button, prepares the next display pass and
sleeps 10 ms — roughly 100 passes per second (shown as "impressions/sec"). Time-based work is
hung off a change of `second()`, with per-second, per-minute, per-hour and per-day cadences:

| Cadence | Work |
|---|---|
| Second | Blanking evaluation (PIR + schedule), temporary-display countdown, NTP update check. |
| Minute | Re-read the RTC into internal time, update uptime/tube-on statistics. |
| Hour | Reconnect WiFi if the connection has been dropped. |
| Day | Persist statistics to SPIFFS, update the day-of-week backlight colour. |

### Time sources

NTP is queried asynchronously; a reply sets the internal clock and is immediately written to the
RTC. An NTP time counts as valid for twice the configured update interval — after that the
clock falls back to the RTC (read once a minute), and if no RTC is fitted, to the internal
clock alone. Local time and DST come from a POSIX TZ string passed to `setenv("TZ", …)`, so the
clock follows DST without any firmware changes.

### Configuration storage

`spiffs_config_t` (in `SPIFFS.h`) is the single source of truth for configuration. It is
written to `/config.json` when the button menu is exited, when a web form is submitted, and on
a factory reset. Usage statistics (`uptimeMins`, `tubeOnTimeMins`) are kept in `/stats.json` and
saved once a day. If `/config.json` cannot be read at boot, the firmware performs a factory
reset; if SPIFFS cannot be mounted at all, the first boot diagnostic LED stays red.

## Web endpoints

All pages except `/`, `/setvalue` and `/local.css` are behind HTTP basic auth when web
authentication is enabled (default on, `admin`/`setup`).

| Path | Purpose |
|---|---|
| `/` | Status summary: network, time source, LDR/brightness, I²C scan, ESP8266 details. |
| `/time` | NTP pool, update interval, TZ string. |
| `/clockconfig` | All display, dimming, blanking, backlight, separator and security settings. |
| `/utility` | Links to the maintenance actions below. |
| `/setvalue` | Show an arbitrary value on the tubes — see the [user manual](USER_MANUAL.md#12-showing-your-own-value-setvalue). |
| `/ntpupdate` | Force an NTP poll now. |
| `/resetwifi` | Clear WiFi credentials and restart. |
| `/reset` | Clear WiFi credentials *and* factory reset the configuration, then restart. |
| `/factoryreset` | Factory reset the configuration only. |
| `/update` | OTA firmware upload (separate credentials: `admin`/`update`). |
| `/local.css` | The inline stylesheet, served locally so the UI works without internet access. |

## Firmware pin map

Defined in `ClockDefs.h` — listed here because the constants matter when reading the code.

| Signal | GPIO | Wemos pin |
|---|---|---|
| Button | 0 | D3 |
| PIR / motion input | 2 | D4 |
| WS2812B data out | 3 | RX (driven by the NeoPixelBus DMA output) |
| LDR | ADC0 | A0 |
| Shift register latch | 14 | D5 |
| Shift register clock | 12 | D6 |
| Shift register data | 13 | D7 |
| Separator LED 1 | 15 | D8 |
| Separator LED 2 | 16 | D0 |
| I²C SDA (RTC) | 4 | D2 |
| I²C SCL (RTC) | 5 | D1 |

Because the LED data line is the RX pin, serial debug output and the backlights cannot be used
at the same time.

## Credits

Written by Ian Sparkes — <nixie@protonmail.ch> — https://www.nixieclock.biz

## Licence

Copyright © 2018–2026 Ian Sparkes. All rights reserved.

This project — both the firmware in [`Software`](Software) and the hardware design in
[`Hardware`](Hardware) — is **free for non-commercial use**. Build one for yourself, modify
it, use it in education or research, and share your changes, as long as you keep the
copyright notices and pass on the same terms.

**Commercial use requires a licence from the author.** That includes selling boards, kits or
finished clocks built from this design, including it in a product or paid service, or using
it in the course of a business. Commercial licences are available on reasonable terms —
contact <nixie@protonmail.ch>.

See [`LICENSE`](LICENSE) for the full terms, and [`Hardware/COPYRIGHT.txt`](Hardware/COPYRIGHT.txt)
for the notice covering the design files. Third-party libraries used by the firmware remain
under their own licences.

> **Safety:** this design runs at high voltage (typically 170–200 V DC). Building and operating
> it is entirely at your own risk; no warranty of any kind is given.
