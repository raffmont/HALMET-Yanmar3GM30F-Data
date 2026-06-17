# Architecture

The project follows a PlatformIO/SensESP HALMET firmware shape and implements Yanmar 3GM30F engine data acquisition.

## Data flow

```text
HALMET inputs -> firmware sampling/filtering -> calibrated values
             -> SensESP SKOutput -> Signal K websocket deltas
             -> NMEA2000 library -> CAN / NMEA 2000 PGNs
             -> SSD1306 OLED -> local status and IP display
```

Optional vibration monitoring extends this flow without changing existing engine, tank, temperature, NMEA 2000, OLED, or Signal K behavior unless it is explicitly enabled:

```text
Engine block accelerometer / IMU -> fixed-rate sampling task
                                  -> DC removal, Hann window, FFT
                                  -> HALMET vendor metrics in Signal K
                                  -> reduced spectrum JSON
                                  -> Signal K plugin / web graph
```

## Modules

- `include/board_pins.h`: HALMET and extra GPIO mapping.
- `include/signalk_paths.h`: schema-valid Signal K path constants.
- `include/nmea2000_mapping.h`: PGN identifiers and transmit periods.
- `include/yanmar_config.h`: device identity, calibration, channel mapping.
- `include/config.h`: compatibility umbrella header.
- `src/main.cpp`: firmware setup, polling, RPM ISR, calibration, OLED status display, Signal K output, and NMEA 2000 transmit loop.
- `signalk/`: Signal K server plugin, diagnostics API, and web GUI, including optional vibration spectrum display.

## Timing

- RPM is counted by interrupt and evaluated with a 1 s gate.
- Signal K values are published every 1 s.
- The OLED status page is refreshed with the Signal K publish loop.
- 1-Wire temperatures are requested every 5 s.
- Optional vibration spectra should be reduced and published at about 1 Hz, with high-rate raw acceleration kept inside firmware.
- PGN 127488 is sent every 100 ms.
- PGN 127489 is sent every 500 ms.
- PGN 127493 is sent every 500 ms.
- PGN 127505 is sent every 2500 ms.
