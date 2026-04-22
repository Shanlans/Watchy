# Project Environment Spec

## Metadata
- Module: PROJECT environment baseline
- Path: master_spec/env_spec.md
- Owner: Shanlan
- Last updated: 2026-04-22

## 1. Goal
- Centralize required local build/run environment information.
- Define how the agent discovers missing tools and persists resolved paths.

## 2. Environment Baseline
- OS: Windows 10 Home 10.0.19045
- Chip: ESP32-S3 (Watchy S3 smartwatch)
- Display: 200x200 E-Ink (GxEPD2)
- Framework: Arduino (ESP32 Arduino Core 3.1.3)

## 3. Known Tool Paths
- Python: C:\Users\Shanlan\miniconda3\python.exe (3.13.9)
- arduino-cli: C:\Users\Shanlan\.local\bin\arduino-cli.exe (1.4.1)
- esptool: pip installed (5.2.0)
- platformio: pip installed (6.1.19) — NOTE: xtensa toolchain 8.4.0 crashes on Windows 10 with heap corruption (0xc0000374), use arduino-cli with Core 3.x instead
- ESP32 Arduino Core: 3.1.3 (installed via arduino-cli, path: %LOCALAPPDATA%\Arduino15\packages\esp32)
- Flash Download Tool: v3.9.7 (manual download from espressif.com)

## 4. Build Commands
- Compile (arduino-cli):
  ```
  arduino-cli compile --fqbn "esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc" <sketch_dir>
  ```
- Upload (esptool):
  ```
  esptool --chip esp32s3 --port COMx --baud 921600 write_flash 0x0 <firmware.merged.bin>
  ```

## 5. Arduino Libraries Installed
- GxEPD2 1.6.9
- Adafruit GFX Library 1.12.6
- Adafruit BusIO 1.17.4
- Arduino_JSON 0.2.0
- NTPClient 3.2.1
- WiFiManager 2.0.17
- DS3232RTC 3.1.2
- Time 1.6.1
- SparkFun MAX3010x 1.1.2
- Rtc_Pcf8563 1.0.3 (git)
- Watchy 1.4.15 (local library)

## 6. Known Issues
- ESP32 Arduino Core 2.x toolchain (xtensa 8.4.0+2021r2-patch5) crashes with `exit status 0xc0000374` on Windows 10. Use Core 3.x instead.
- PlatformIO uses `@tempfile` response files that silently swallow compiler errors on Windows. Use arduino-cli as workaround.
- Watchy library `Watchy.cpp` references `MAX3010x.h` but library provides `MAX30105.h` — patched in local Arduino library.

## 7. Environment Variables And Resolution Policy
- If a required tool/path/env is missing during task execution, the agent must:
  1. Detect the missing dependency from command failure or precheck.
  2. Auto-search locally first.
  3. If still unresolved, ask the user.
  4. Persist the resolved value into this file once confirmed usable.
