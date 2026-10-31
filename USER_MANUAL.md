# ESP8266 4-Digit Nixie Clock — User Manual

For the firmware in [`Software`](Software), software version `8`.
Developer-facing documentation is in [`README.md`](README.md).

## Contents

1. [Overview](#1-overview)
2. [First power-on](#2-first-power-on)
3. [Connecting to WiFi](#3-connecting-to-wifi)
4. [Finding the clock on your network](#4-finding-the-clock-on-your-network)
5. [The button](#5-the-button)
6. [Temporary displays](#6-temporary-displays)
7. [The configuration menu](#7-the-configuration-menu)
8. [Blanking and waking the clock](#8-blanking-and-waking-the-clock)
9. [The web interface](#9-the-web-interface)
10. [Time, timezone and DST](#10-time-timezone-and-dst)
11. [Display effects](#11-display-effects)
12. [Showing your own value (`/setvalue`)](#12-showing-your-own-value-setvalue)
13. [Over-the-air firmware updates](#13-over-the-air-firmware-updates)
14. [Resetting the clock](#14-resetting-the-clock)
15. [Troubleshooting](#15-troubleshooting)

---

## 1. Overview

The clock has four tubes, two separator LEDs, an RGB backlight under each tube, and a single
button. It gets the time from the internet (NTP) and, if a DS1307 real time clock chip is
fitted, keeps it over power cuts and internet outages.

Everything can be configured in two ways:

- **From the button** — a 36-position menu, driven entirely by short and long presses.
- **From a web browser** — the clock runs a small web server on your network.

Both change the same settings, and settings are saved permanently in the clock's flash memory.

---

## 2. First power-on

When power is applied the clock runs through a power-on self test. The tubes count **0, 1, 2 …
16** while the checks run, and the backlight LEDs act as status indicators — the first LED is
the flash filesystem, the second WiFi, the third NTP, the fourth the real time clock:

| Colour | Meaning |
|---|---|
| Yellow | This check is in progress |
| Green | Passed |
| Red | Failed |
| Blue | Needed attention — configuration was reset, or the WiFi setup portal was opened |

A red LED is not necessarily fatal: a red third LED just means no time was received from the
internet yet, and a red fourth LED simply means no RTC chip is fitted.

Startup takes around 20 seconds. The version number is then shown briefly and the clock starts
keeping time.

---

## 3. Connecting to WiFi

If the clock has no WiFi credentials stored, it opens its own access point during startup:

1. On your phone or laptop, join the WiFi network named after the clock's hostname —
   typically something like `ESP_a1b2c3`.
2. Password: **`SetMeUp!`**
3. A configuration page should open automatically. If it does not, browse to
   `http://192.168.4.1`.
4. Choose your network, enter its password, and save.

The clock restarts and joins your network. The portal times out after **60 seconds** — if
nothing connects in that time, the clock carries on without WiFi (running on the RTC if one is
fitted), so power-cycle it to try again.

---

## 4. Finding the clock on your network

**On the tubes.** With the clock showing the time, press the button briefly to step through the
temporary displays until you reach the four IP address octets. Each is shown for 5 seconds with
the leftmost tube blank, so `192.168.1.57` appears as ` 192`, ` 168`, `   1`, `  57`.

**By name.** The clock registers itself with mDNS, so on most machines
`http://<clockname>.local` works — the clock's name is shown as "Clock Name" on the web
summary page.

Browse to the IP address or name in any browser. Pages other than the summary page ask for a
user name and password; the defaults are:

- User name: **`admin`**
- Password: **`setup`**

Change these under *Configure clock settings → Security*.

---

## 5. The button

The firmware distinguishes four press lengths. The display always *previews* what a long press
will do while you hold the button — the action happens when you let go.

| Press | In normal time display | In the configuration menu |
|---|---|---|
| Short (under 1 s) | Step through the temporary displays, or wake the clock if it is blanked | Change the value of the current setting |
| Medium (1–2 s) | Enter the configuration menu | Move to the next menu position |
| Long (over 2 s) | — | Leave the menu, save settings, return to the time |

Holding the button does nothing destructive — if you are unsure where you are, hold the button
for more than two seconds and release to get back to the time display.

Settings are written to flash when you leave the menu, so always exit the menu with a long
press rather than pulling the power.

---

## 6. Temporary displays

With the time on display, each short press shows something else for 5 seconds. Press again
within those 5 seconds to move to the next one:

| Order | Shows |
|---|---|
| 1 | Minutes and seconds |
| 2 | The date, in the configured format |
| 3 | The light sensor reading |
| 4–7 | The four octets of the clock's IP address |
| 8 | Display passes per second (a health indicator, normally around 100) |

If the clock is not connected to WiFi, the IP address entries are skipped.

---

## 7. The configuration menu

From the time display, press the button for 1–2 seconds and release. The two left tubes show
the **menu position number**, blinking, and the two right tubes show the **current value**,
brightly lit. Short presses change the value; another 1–2 second press moves on; a press of over
2 seconds saves and exits.

Positions that do not apply are skipped automatically — for example the colour channels only
appear when the backlight is set to a fixed colour, and the blanking hours only appear when a
blanking mode that uses hours is selected.

### Setting the time and date (positions 1–6)

These are **only shown when the clock has no valid time from the internet**. If NTP is working,
the menu jumps straight from the time display to position 7.

| Pos | Sets | Notes |
|---|---|---|
| 1 | Hours | All four tubes show the time, the hours blink |
| 2 | Minutes | The minutes blink |
| 3 | Seconds | Each press resets the seconds to zero |
| 4 | Day | Shown in the configured date format |
| 5 | Month | |
| 6 | Year | |

### All other settings (positions 7–36)

| Pos | Setting | Values | Default |
|---|---|---|---|
| 07 | 12/24 hour mode | 0 = 24 hour, 1 = 12 hour | 0 |
| 08 | Blank leading zero | 0 = show, 1 = blank | 0 |
| 09 | Scrollback effect | 0 = off, 1 = on | 0 |
| 10 | Scroll steps, more — slower scroll | 1–80 | 8 |
| 11 | Scroll steps, fewer — faster scroll | as above, counts the other way | |
| 12 | Fade effect | 0 = off, 1 = on | 0 |
| 13 | Fade steps, more — longer fade | 20–200 (100 ≈ 1 second) | 50 |
| 14 | Fade steps, fewer — shorter fade | as above, counts the other way | |
| 15 | Date format | 0 = MM-DD, 1 = DD-MM | 1 |
| 16 | Day blanking mode | 0–8, see below | 0 |
| 17 | Blank from (hour) | 0–23 | 0 |
| 18 | Blank until (hour) | 0–23 | 7 |
| 19 | Use light sensor | 0 = off (always full brightness), 1 = on | 1 |
| 20 | Minimum brightness, increase | 2–20 | 4 |
| 21 | Minimum brightness, decrease | as above, counts the other way | |
| 22 | What blanking switches off | 0 = tubes, 1 = LEDs, 2 = tubes + LEDs, 3 = everything | 2 |
| 23 | Date "slots" animation | 0 = off, 1 = scroll in / scramble out | 1 |
| 24 | Separator LED mode | 0–6, see below | 0 |
| 25 | Motion timeout, increase | 60–3600 seconds, in steps of 10 | 300 |
| 26 | Motion timeout, decrease | as above, counts the other way | |
| 27 | Motion sensor pull-up | 0 = off, 1 = on | 1 |
| 28 | Backlight mode | 0 = fixed, 1 = cycle, 2 = colour by digit, 3 = colour by day | 1 |
| 29 | Red intensity | 0–15 (fixed mode only) | 15 |
| 30 | Green intensity | 0–15 (fixed mode only) | 0 |
| 31 | Blue intensity | 0–15 (fixed mode only) | 0 |
| 32 | Cycle speed | 4–64, in steps of 2 (cycle mode only) | 10 |
| 33 | Backlight pulse | 0 = off, 1 = pulse once per second | 0 |
| 34 | Backlight follows light sensor | 0 = off, 1 = on | 1 |
| 35 | Software version | read only | 8 |
| 36 | Tube test | all four tubes show the same changing digit | |

Positions 25 and 26 use all four tubes to show the timeout in seconds, since it does not fit in
two digits.

### Day blanking modes (position 16)

| Value | Blanks |
|---|---|
| 0 | Never |
| 1 | All day at weekends |
| 2 | All day on week days |
| 3 | Always |
| 4 | Between the selected hours, every day |
| 5 | Between the selected hours on week days, and all day at weekends |
| 6 | Between the selected hours at weekends, and all day on week days |
| 7 | Between the selected hours, at weekends only |
| 8 | Between the selected hours, on week days only |

Setting "blank from" and "blank until" to the same hour disables hour-based blanking. Setting
"from" later than "until" blanks across midnight.

### Separator LED modes (position 24)

| Value | Behaviour |
|---|---|
| 0 | Railroad — the two LEDs alternate once per second |
| 1 | Slow flash — both on for half a second, off for half a second |
| 2 | Fast flash |
| 3 | Double flash |
| 4 | Always on |
| 5 | Always off |
| 6 | AM/PM — the top LED is lit in the morning, the bottom in the afternoon |

---

## 8. Blanking and waking the clock

Blanking switches the display off to save the tubes. It is driven by one of two things:

- **The schedule** you set at menu position 16, or
- **A motion sensor**, if one is fitted. The firmware detects the sensor automatically the
  first time it reports; from then on the schedule is ignored and the display is simply blanked
  when nobody has been seen for the configured timeout (default 5 minutes).

What actually switches off — tubes, backlights, separators — is set at menu position 22.

### Waking it up temporarily

Press the button while the clock is blanked. Each press within five seconds of the last extends
the override:

| Presses | Display stays on for |
|---|---|
| 1 | 10 seconds |
| 2 | 1 hour |
| 3 | 4 hours |

After that the clock goes back to its normal schedule by itself.

---

## 9. The web interface

Four pages, reachable from the navigation bar at the top.

### Summary (`/`)

Read-only status. Network details (IP, MAC, SSID, clock name), the NTP pool and timezone in
use, when the last time update arrived and when the next is due, uptime, the result of an I²C
bus scan, the current light sensor reading and resulting digit brightness, the motion sensor
state, which time source is in use, the RTC's own time, display passes per second, lifetime
clock-on and tube-on hours, and ESP8266 memory/flash details.

"Total Tube On Hrs" counts only time when the tubes were actually lit, so it is a genuine
measure of tube wear.

### Configure Time Server (`/time`)

- **NTP Pool** — the time server to use, by default `pool.ntp.org`.
- **Update interval** — seconds between polls, 60 to 86400. The default is 7261 (just over two
  hours), deliberately not a round number so that many clocks do not poll in step.
- **Time Zone String** — see [section 10](#10-time-timezone-and-dst).

### Configure clock settings (`/clockconfig`)

Everything from the button menu, plus a few settings that are only available here:

| Section | Contains |
|---|---|
| General options | 12/24 hour, leading zero, date format, date slots animation, scrollback and its speed, fade and its length |
| Light Dependent Resistor | Use the sensor or not, minimum brightness, **LDR threshold** (raises the base brightness), **LDR sensitivity** (how strongly brightness tracks the light level) |
| Digit blanking | Motion timeout and pull-up, day blanking mode, what blanking switches off, blanking hours |
| Back/Underlights | Backlight mode, cycle speed, dim with the light sensor, pulse, fixed red/green/blue intensities, **backlight dim** (an overall backlight brightness limit, 10–100%) |
| Separators | Separator LED mode and **separator dimming** (off, dim, medium, bright, full) |
| Security | Web authentication on/off, user name, password |

Each section has its own **Set** button and saves independently.

### Utility (`/utility`)

Links to: restart the WiFi module, clear the WiFi configuration and restart, update the
firmware, force an NTP update now, and perform a factory reset.

---

## 10. Time, timezone and DST

The clock stores time internally in UTC-free local form and relies on a **POSIX timezone
string** for the local offset and daylight saving rules. The default is:

```
CET-1CEST,M3.5.0,M10.5.0/3
```

which is Central European Time with EU summer time. Other common examples:

| Zone | String |
|---|---|
| UK | `GMT0BST,M3.5.0/1,M10.5.0` |
| US Eastern | `EST5EDT,M3.2.0,M11.1.0` |
| US Pacific | `PST8PDT,M3.2.0,M11.1.0` |
| Australia (NSW) | `AEST-10AEDT,M10.1.0,M4.1.0/3` |
| No DST, UTC | `UTC0` |

Because the rules come from this string, daylight saving changes happen by themselves and
survive future rule changes without a firmware update.

**Which source is in use** is shown on the summary page:

- **NTP** — a time from the internet, still considered fresh. It stays valid for twice the
  update interval.
- **RTC** — the battery-backed clock chip, re-read once a minute. Used when NTP is stale.
- **Internal** — the ESP8266's own timer. Used only when there is neither NTP nor an RTC; it
  will drift.

Every NTP update is written to the RTC, so the RTC stays accurate as long as the clock
occasionally sees the internet.

---

## 11. Display effects

**Fade** crossfades each tube from the old digit to the new one. The fade length is in display
passes — roughly 100 passes per second, so the default of 50 is about half a second.

**Scrollback** applies when a digit rolls over to zero: instead of changing straight to 0, it
counts down through the digits. The scroll setting is how many display passes each step takes,
so a lower number scrolls faster.

Fade and scrollback can both be on: scrollback is used on roll-over to zero, fade on every
other change.

**Date slots** briefly replaces the time with the date at 50 seconds past each minute: the date
scrolls in, holds, then scrambles back out to the time.

**Backlight modes:**

- **Fixed** — one colour, set with the red/green/blue intensities (0–15 each).
- **Cycle** — drifts smoothly through random colours; the cycle speed sets how fast.
- **Colour by digit** — each tube's colour follows the digit it is showing, 0 through 9.
- **Colour by day** — one colour for the whole clock, changing with the day of the week.

**Pulse** brightens and dims the backlight once per second in time with the seconds.
**Dim backlights** makes the backlight follow the ambient light sensor along with the tubes.

---

## 12. Showing your own value (`/setvalue`)

The clock can display an arbitrary four-digit number on request — useful for home automation,
scripts or a temperature feed. This endpoint deliberately requires no password.

```
http://<clock-ip>/setvalue?value=1234&time=60&format=3333
```

| Parameter | Required | Meaning |
|---|---|---|
| `value` | yes | The number to show. Only the last four digits are used. |
| `time` | no | How many seconds to show it for, 1–255. Defaults to 10. **255 means show it until something changes it.** |
| `format` | no | Four digits, one per tube from left to right, choosing how each tube is displayed. |

Format digits:

| Digit | Tube appearance |
|---|---|
| 0 | Blank |
| 1 | Dimmed |
| 2 | Normal (follows the light sensor) |
| 3 | Fading |
| 4 | Scrolling |
| 5 | Blinking |
| 6 | Full brightness |

So `format=6655` shows the first two tubes at full brightness and the last two blinking.

---

## 13. Over-the-air firmware updates

Once the clock is on your network, new firmware can be uploaded without a USB cable.

1. Build the firmware with `pio run`. The `.bin` file is written to
   `.pio/build/d1_mini/firmware.bin`.
2. Browse to `http://<clock-ip>/update`.
3. Log in with user name **`admin`** and password **`update`** — note these are separate from
   the normal web password.
4. Choose the `.bin` file and upload. The clock restarts when it finishes.

Your settings are kept across an update; they live in a separate area of flash.

---

## 14. Resetting the clock

| Goal | How |
|---|---|
| Forget the WiFi network only | *Utility → Restart WiFi module* at `/resetwifi` |
| Reset all settings, keep WiFi | *Utility → Perform factory reset* at `/factoryreset` |
| Reset everything, including WiFi | *Utility → Clear WiFi config and restart* at `/reset` |
| Reset everything with no network access | Power the clock on, then press and hold the button as soon as the tubes start counting up. The first status LED turns blue and the clock restarts with default settings. |

A factory reset restores the defaults listed in [section 7](#7-the-configuration-menu),
including the web user name `admin` and password `setup`.

---

## 15. Troubleshooting

**The tubes stop at a number during startup.** The number is the step the power-on self test
reached. Step 8 is the WiFi connection and step 9 the first NTP poll — these are the slow ones
and can take 10–20 seconds each. If it stays on one number for much longer, power-cycle the
clock; if it stops at the same number again, that step is failing.

**The first status LED is red.** The flash filesystem could not be mounted, so settings cannot
be saved. The clock will still run, but every restart comes up with defaults. The firmware
needs to be flashed with a SPIFFS partition (4M with 1M SPIFFS).

**The second status LED is blue.** The clock could not join a known network and opened its own
setup access point — see [section 3](#3-connecting-to-wifi).

**The time is wrong, or drifts.** Check the summary page's "Time Source". If it says *Internal*
the clock is free-running with neither the internet nor an RTC, and will drift. If it says
*NTP* but the time is out by a whole number of hours, the timezone string is wrong — see
[section 10](#10-time-timezone-and-dst).

**The display blanks when it should not.** If a motion sensor has ever reported, it takes over
from the schedule completely. The summary page shows "Motion Sensor" with the time since motion
was last seen. Either raise the motion timeout, or press the button to override the blanking
for up to four hours.

**The display never blanks.** Blanking only applies while the clock is showing the time — it
is suppressed in the configuration menu and during temporary displays.

**The clock is too dim, or too bright.** Raise the minimum brightness (menu position 20) to set
a floor. On the web page, "LDR Threshold" lifts the whole brightness range and "LDR
Sensitivity" changes how strongly it reacts to ambient light. Switching the light sensor off
entirely runs the tubes at full brightness.

**The backlights are off or the wrong colour.** Check the backlight mode: red/green/blue
intensities only apply in *fixed* mode. Also check "Backlight Dim" on the web page, which caps
overall backlight brightness, and whether blanking is set to switch the LEDs off.

**"Impressions/Sec" is much lower than about 100.** The main loop is being held up — usually by
a busy web client or a poor WiFi connection. Fading and scrolling will look slower than
configured, since their speeds are measured in display passes.

**I have forgotten the web password.** Do the button-held-at-startup factory reset described in
[section 14](#14-resetting-the-clock); this restores `admin` / `setup`.

---

## Licence and copyright

Copyright © 2018–2026 Ian Sparkes. All rights reserved.
Free for non-commercial use; commercial use requires a licence from the author.
See `LICENSE` in the repository root. Contact: nixie@protonmail.ch — https://www.nixieclock.biz

This clock runs at high voltage internally. Do not open the case while it is powered.
