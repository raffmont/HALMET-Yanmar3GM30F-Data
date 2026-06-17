# Signal K mapping

The firmware publishes only schema-valid Signal K paths. Engine-panel warning lamp inputs are represented on NMEA 2000 as engine discrete-status bits rather than as ad-hoc Signal K boolean children under numeric engine paths.

## Paths

| Source | Signal K path | Units | Notes |
| --- | --- | ---: | --- |
| Flywheel optocoupler on GPIO17 | `propulsion.main.revolutions` | Hz | Signal K uses revolutions per second. RPM is `value * 60`. |
| Derived from RPM and/or D4 | `propulsion.main.state` | enum | `started` when RPM is detected or D4 is active, otherwise `stopped`. |
| A1 | `tanks.fuel.main.currentLevel` | ratio | Fuel level, 0.0 to 1.0. |
| A2 | `propulsion.main.coolantTemperature` | K | Calibrated coolant temperature. |
| A3 | `propulsion.main.oilPressure` | Pa | Calibrated oil pressure. |
| A4 | `propulsion.main.alternatorVoltage` | V | Alternator or charging voltage. |
| 1-Wire sensor 1 default | `environment.inside.engineRoom.temperature` | K | Engine-room air temperature. |
| 1-Wire sensor 2 default | `propulsion.main.exhaustTemperature` | K | Exhaust elbow surface proxy temperature. |
| 1-Wire sensor 3 default | `propulsion.main.transmission.oilTemperature` | K | Gearbox case surface proxy temperature. |
| 1-Wire sensor 4 default | `electrical.alternators.main.temperature` | K | Alternator case temperature. |

## Removed non-schema helper paths

The project intentionally does not publish:

- `propulsion.main.revolutions.rpm`
- `propulsion.main.oilPressure.alarm`
- `propulsion.main.coolantTemperature.alarm`
- `electrical.alternators.main.alarm`
- `propulsion.main.panel.aux`
- raw voltage paths under standard engine/tank branches

Raw voltages remain local firmware variables. If raw diagnostics are needed, add a documented vendor extension rather than placing them under standard Signal K branches.

## Unit conventions

- Engine revolutions: hertz, not RPM.
- Temperature: kelvin, not Celsius.
- Pressure: pascal, not bar or psi.
- Tank level: ratio, not percent.

## Server-side plugin

The repository also includes a Signal K server plugin in `signalk/`.

The plugin does not create private diagnostic Signal K branches. Diagnostics are exposed through HTTP JSON endpoints, while derived vessel data is limited to schema paths:

| Derived value | Signal K path | Units |
| --- | --- | ---: |
| Fuel remaining | `tanks.fuel.main.remaining` | m³ |
| Engine state fallback | `propulsion.main.state` | enum |

The GUI converts values for display only: Hz to RPM, kelvin to Celsius, pascal to bar, and fuel ratio to percent.
