---
name: watchy-build-setup
description: Watchy S3 build environment setup for Arduino IDE 2.x / arduino-cli. Records FQBN, tool paths, required libraries, and known Watchy library patches. Use to avoid reconfiguring tools and libraries on each session.
---

# Watchy S3 Build Environment Setup

Use this skill when (re)configuring the build toolchain for Watchy S3 firmware, or when diagnosing build-environment regressions.

## Trigger
- Keywords: "配置编译环境" / "setup build" / "arduino ide watchy"
- A build fails due to missing library, wrong FQBN, or toolchain version mismatch.
- A fresh machine or fresh Arduino install needs bring-up.

## Hardware Facts

| Field | Value |
|---|---|
| Chip | ESP32-S3 |
| Flash | 16 MB (qio_qspi) |
| PSRAM | disabled |
| Display | 200x200 E-Ink, GxEPD2 driver |
| USB | Hardware USB CDC + JTAG (native USB Serial/JTAG, no CH340) |
| Heart-rate sensor | MAX30102 (requires `MAX30102_PulseOximeter.h` API, currently stubbed) |

## Canonical FQBN

```
esp32:esp32:esp32s3:UploadSpeed=921600,USBMode=hwcdc,CDCOnBoot=cdc,MSCOnBoot=default,DFUOnBoot=default,UploadMode=default,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,DebugLevel=none,PSRAM=disabled,LoopCore=1,EventsCore=1,EraseFlash=none,JTAGAdapter=default,ZigbeeMode=default
```

Short form (omits defaults — sufficient for arduino-cli):
```
esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB
```

## Arduino IDE 2.x GUI Settings

After opening a Watchy sketch, set via `Tools` menu:

| Menu Item | Required Value |
|---|---|
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | Enabled |
| USB Mode | Hardware CDC and JTAG |
| Flash Size | 16MB (128Mb) |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| CPU Frequency | 240MHz (WiFi) |
| Flash Mode | QIO |
| PSRAM | Disabled |
| Core Debug Level | None |
| Upload Speed | 921600 |

Leave the rest at defaults (UploadMode=default, DFUOnBoot=default, MSCOnBoot=default, JTAGAdapter=default, ZigbeeMode=default, EraseFlash=none, LoopCore=1, EventsCore=1).

## Tool Versions (known-good)

| Tool | Version | Path |
|---|---|---|
| Arduino IDE | 2.3.8 | `E:\CP\ArduinoIDE\Arduino IDE.exe` (portable) |
| arduino-cli (bundled + standalone) | 1.4.1 | `%USERPROFILE%\.local\bin\arduino-cli.exe` |
| ESP32 Arduino Core | **3.0.7** (known-good on Windows) | `%LOCALAPPDATA%\Arduino15\packages\esp32\hardware\esp32\3.0.7` |
| xtensa toolchain | esp-xs3/2302 (gcc 12.2.0) | `%LOCALAPPDATA%\Arduino15\packages\esp32\tools\esp-xs3\2302\bin` |
| esptool | 4.9.dev3 (bundled) / 5.2.0 (pip) | bundled with core / `pip install esptool` |
| Python | 3.13.9 | `C:\Users\Shanlan\miniconda3\python.exe` |

**Avoid on Windows:**
- ESP32 Core 2.x — xtensa 8.4.0 crashes with `0xc0000374` heap corruption during compile
- ESP32 Core 3.3.8 — GCC 14.2.0 + `-mdynconfig=xtensa_esp32s3.so` silently crashes cc1plus.exe
- ESP32 Core 3.1.x–3.2.x — ld.exe crashes linking 170+ object files (heap corruption)

**Known-good on Windows 10:** `esp32:esp32@3.0.7` with GCC 12.2.0. Linker still crashes if too many objects, so **GxEPD2 must be trimmed** (see below).

## Required Arduino Libraries

Install via `Library Manager` (Arduino IDE) or `arduino-cli lib install`:

| Library | Version | Install Command |
|---|---|---|
| GxEPD2 | 1.6.9+ | `arduino-cli lib install GxEPD2` |
| Adafruit GFX Library | 1.12.6+ | `arduino-cli lib install "Adafruit GFX Library"` |
| Adafruit BusIO | 1.17.4+ | `arduino-cli lib install "Adafruit BusIO"` |
| Arduino_JSON | 0.2.0+ | `arduino-cli lib install Arduino_JSON` |
| NTPClient | 3.2.1+ | `arduino-cli lib install NTPClient` |
| WiFiManager | 2.0.17+ | `arduino-cli lib install WiFiManager` |
| DS3232RTC | 3.1.2+ | `arduino-cli lib install DS3232RTC` |
| Time | 1.6.1+ (dep of DS3232RTC) | auto-installed |
| Rtc_Pcf8563 | 1.0.3+ | `arduino-cli lib install --git-url https://github.com/orbitalair/Rtc_Pcf8563.git` (requires `library.enable_unsafe_install: true` in arduino-cli config) |

**Watchy library** (the core class itself) must be copied to Arduino libraries folder:
```
Source: Watchy-S3Watch/watchySrc/Watchy/Watchy/
Target: %USERPROFILE%\Documents\Arduino\libraries\Watchy\
```

### Libraries NOT to install
These conflict or are outdated:
- `SparkFun MAX3010x Pulse and Proximity Sensor Library` — API mismatch with Watchy heart-rate code
- `MAX30100lib` (oxullo) — Watchy expects MAX30102-specific API, not MAX30100

## Library Patches (known-good state)

| File | Patch | Reason |
|---|---|---|
| `~/Documents/Arduino/libraries/Watchy/src/Watchy.cpp` line ~3 | Comment out `#include <MAX30105.h>` and `#include "MAX30102_PulseOximeter.h"` | No compatible MAX30102 Arduino library on Core 3.x yet |
| `~/Documents/Arduino/libraries/Watchy/src/Watchy.cpp` `showHeartrate()` | Replace with stub that shows "MAX30102 lib needed"; wrap original body in `#if 0 ... #endif` | Heart-rate feature disabled until MAX30102 library adapted |
| `~/Documents/Arduino/libraries/Watchy/src/WatchyRTC.cpp` | Wrap entire body in `#ifndef ARDUINO_ESP32S3_DEV ... #endif` (and `#include "config.h"` at top) | Rtc_Pcf8563 library uses `WireBase` type not available in Arduino-ESP32; file gets compiled as standalone TU even though Watchy.h only includes it for non-S3 builds |
| GxEPD2 library | Move all `src/epd*/`, `src/gde*/`, `src/it8951/`, `src/other/` `.cpp` except `epd/GxEPD2_154_D67.cpp` into a folder **outside** `src/` (e.g. `_disabled_drivers/`) | Arduino compiles all `.cpp` recursively under `src/`. 170+ display drivers triggers `ld.exe` heap corruption (`0xc0000374`) on Windows at link time. Watchy only needs `GDEH0154D67` panel. Keep `GxEPD2_EPD.cpp` (base class) + `epd/GxEPD2_154_D67.cpp`. |

Watchy code lives in: `%USERPROFILE%\Documents\Arduino\libraries\Watchy\src\Watchy.cpp`

## Project Sketch Location

Canonical sketch folder: `E:\CP\Watchy\firmware\arduino\Watchy_7SEG\`

Files: `Watchy_7SEG.ino`, `Watchy_7_SEG.cpp/.h`, `settings.h`, font headers, `icons.h`.

## Verify Setup (quick smoke)

```powershell
# 1. Check core
arduino-cli core list
# Expect: esp32:esp32  3.1.3

# 2. Check libs
arduino-cli lib list | Select-String "Watchy|GxEPD2|WiFiManager|Rtc_Pcf8563"

# 3. Compile test
arduino-cli compile --fqbn "esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB" "E:\CP\Watchy\firmware\arduino\Watchy_7SEG"
```

## arduino-cli Config

```
# %USERPROFILE%\AppData\Local\Arduino15\arduino-cli.yaml (add/keep)
board_manager:
  additional_urls:
    - https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
library:
  enable_unsafe_install: true
```

## Known Bug (workaround)

**Symptom:** `arduino-cli compile` on Windows exits 1 with only `Error during build: exit status 1` in stderr; no `error:` lines visible.

**Cause:** arduino-cli 1.4.1 on Windows loses stderr from xtensa compiler subprocess.

**Workaround:** Use Arduino IDE 2.x GUI — its gRPC-based output panel captures compiler stderr via a different channel, so real errors appear.

## Related
- Flashing: see `skill/firmware-flash/SKILL.md`
- Project profile: `project_profile.yaml`
- Environment: `master_spec/env_spec.md`
