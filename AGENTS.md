# Repository guidance

This repository is a PlatformIO/SensESP HALMET firmware project with a companion Signal K server plugin.

## Firmware

- Keep board-level pin constants in `include/board_pins.h`.
- Keep Signal K schema paths in `include/signalk_paths.h`.
- Keep NMEA 2000 PGN details in `include/nmea2000_mapping.h` and the implementation in `src/main.cpp`.
- Keep user-tunable engine and sensor calibration values in `include/yanmar_config.h`.

## Signal K server plugin

- Keep the plugin in `signalk/`.
- Keep firmware-facing path inventory in `sk-model/`.
- Do not emit non-schema Signal K paths from firmware or plugin code unless they are deliberately documented as private diagnostics.
