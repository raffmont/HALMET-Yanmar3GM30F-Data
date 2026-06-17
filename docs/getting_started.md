# Getting started

## Prerequisites

- HALMET board.
- PlatformIO.
- Signal K server for Wi-Fi/websocket testing.
- NMEA 2000 test backbone or simulator if validating PGNs.
- Bench signal sources before connecting to the Yanmar harness.

## Build and upload

```bash
platformio run
platformio run -t upload
platformio device monitor
```

The default environment is `halmet`, so `platformio run` is enough.

## First boot

1. Power HALMET on the bench.
2. Complete SensESP Wi-Fi onboarding.
3. Pair with the Signal K server.
4. Confirm deltas under the paths listed in `docs/signalk.md`.
5. Connect a safe pulse generator to `GPIO17` and confirm `propulsion.main.revolutions`.
6. Connect simulated or bench analog voltages to A1-A4 and validate scaling.
7. Connect one DS18B20 at a time and record each ROM ID in `include/yanmar_config.h`.
