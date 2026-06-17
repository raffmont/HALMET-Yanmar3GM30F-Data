# HALMET Yanmar 3GM30F Data

HALMET firmware for acquiring Yanmar 3GM30F engine-panel, flywheel RPM, analog sender, fuel-tank, and 1-Wire temperature data and publishing it to Signal K and NMEA 2000.

SensESP provides onboarding, OTA, configuration, Signal K integration, and the cooperative event loop; the application code adds the Yanmar 3GM30F data model and NMEA 2000 engine/tank PGNs.

## Features

- Flywheel RPM sensing on external optocoupler input `GPIO17`.
- D1-D4 Yanmar type-B panel taps for oil-pressure, coolant-temperature, charge-lamp, and auxiliary/key-on state.
- A1-A4 HALMET isolated analog inputs for fuel level, coolant temperature, oil pressure, and alternator voltage.
- 1-Wire DS18B20 temperature probes with deterministic path mapping by ROM ID or sensor order.
- Schema-valid Signal K outputs with SI units.
- NMEA 2000 PGNs `127488`, `127489`, and `127505`.
- PlatformIO `halmet` default environment using pioarduino and SensESP.
- Documentation split into getting-started, architecture, configuration, Signal K, NMEA 2000, hardware, safety, and commissioning files.

## Repository layout

```text
.
├── .vscode/        VS Code / PlatformIO recommendations
├── docs/           Architecture, configuration, Signal K, NMEA 2000, hardware, safety
├── include/        Board pins, Signal K paths, NMEA 2000 mapping, Yanmar configuration
├── sk-model/       Firmware-facing Signal K path inventory
├── signalk/        Signal K server plugin and web GUI
├── src/            HALMET firmware implementation
├── test/           Reserved for PlatformIO tests
├── platformio.ini  PlatformIO build configuration
└── README.md
```

## Build

Install PlatformIO, then run:

```bash
platformio run
```

Upload:

```bash
platformio run -t upload
```

Serial monitor:

```bash
platformio device monitor
```

The project default environment is `halmet`.

## Default hardware mapping

| Function | HALMET / ESP32 |
| --- | --- |
| Flywheel RPM optocoupler | extra input on `GPIO17` |
| Oil pressure panel warning | D1 / `GPIO23` |
| Coolant temperature panel warning | D2 / `GPIO25` |
| Alternator charge warning lamp | D3 / `GPIO27` |
| Key-on / buzzer / spare panel signal | D4 / `GPIO26` |
| Fuel level | A1 / ADS1115 channel 0 |
| Coolant temperature sender | A2 / ADS1115 channel 1 |
| Oil pressure sender | A3 / ADS1115 channel 2 |
| Alternator / charge voltage | A4 / ADS1115 channel 3 |
| I2C SDA/SCL | `GPIO21` / `GPIO22` |
| NMEA 2000 CAN TX/RX | `GPIO19` / `GPIO18` |
| 1-Wire temperature bus | `GPIO4`, verify on board revision |

## Signal K outputs

| Measurement | Signal K path | Units |
| --- | --- | --- |
| Engine speed | `propulsion.main.revolutions` | Hz |
| Engine state | `propulsion.main.state` | enum |
| Fuel tank level | `tanks.fuel.main.currentLevel` | ratio |
| Coolant temperature | `propulsion.main.coolantTemperature` | K |
| Oil pressure | `propulsion.main.oilPressure` | Pa |
| Alternator voltage | `propulsion.main.alternatorVoltage` | V |
| Engine-room temperature | `environment.inside.engineRoom.temperature` | K |
| Exhaust temperature | `propulsion.main.exhaustTemperature` | K |
| Transmission oil/case temperature | `propulsion.main.transmission.oilTemperature` | K |
| Alternator case temperature | `electrical.alternators.main.temperature` | K |

## NMEA 2000 outputs

| PGN | Name | Period | Source |
| ---: | --- | ---: | --- |
| 127488 | Engine Parameters, Rapid Update | 100 ms | Flywheel RPM on `GPIO17` |
| 127489 | Engine Parameters, Dynamic | 500 ms | A2 coolant temperature, A3 oil pressure, A4 alternator voltage, D1-D3 status bits |
| 127505 | Fluid Level | 2500 ms | A1 fuel tank level |

## Documentation

- [Getting started](docs/getting_started.md)
- [Architecture](docs/architecture.md)
- [SensESP configuration](docs/sensesp_configuration.md)
- [Signal K interface](docs/signalk.md)
- [Signal K model inventory](sk-model/README.md)
- [Signal K server plugin and GUI](signalk/README.md)
- [NMEA 2000](docs/nmea2000.md)
- [Hardware notes](docs/hardware.md)
- [Safety notes](docs/safety.md)
- [Commissioning checklist](docs/commissioning.md)

## Safety

This firmware is for monitoring and data acquisition. It must not replace the Yanmar alarm buzzer, lamps, shutdown protection, fusing, or marine-grade wiring practice. Keep added wiring fused, strain-relieved, labelled, reversible, and isolated where required.

## Credits

Uses SensESP, the NMEA2000 library, NMEA2000_twai, Adafruit ADS1X15, OneWire, and DallasTemperature.
