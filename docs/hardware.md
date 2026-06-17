# Hardware notes

## RPM optocoupler on flywheel, GPIO17

Use the extra optocoupler board as a galvanic boundary between the flywheel pickup and the ESP32 input.

```text
Flywheel pickup side                    HALMET logic side
--------------------                    -----------------
Pickup/sensor supply  -> optocoupler -> module logic VCC, 3.3 V preferred
Pickup/sensor signal  -> optocoupler -> GPIO17
Pickup/sensor return  -> isolated    -> HALMET GND only on logic side
```

The firmware configures `GPIO17` as `INPUT_PULLUP` and counts falling edges. If the optocoupler module output is push-pull 5 V, do not connect it directly to the ESP32 pin; configure the module for 3.3 V logic or add level shifting. Set `RPM_PULSES_PER_REV` after deciding whether you sense one mark, multiple marks, or teeth.

## Engine panel type-B taps on D1-D4

Use insulated piggyback crimps, a service connector breakout, or a reversible harness between the Yanmar engine and panel. Do not cut the loom unless absolutely necessary.

| Input | Firmware default | NMEA 2000 output |
| --- | --- | --- |
| D1 | Oil pressure warning | PGN 127489 Engine Status 1 `LowOilPressure` |
| D2 | Coolant temperature warning | PGN 127489 Engine Status 1 `OverTemperature` |
| D3 | Alternator charge lamp | PGN 127489 Engine Status 1 `ChargeIndicator` |
| D4 | SD20 sail drive seal switch alarm | PGN 127493 Transmission Status `Check` and `SailDrive` |

The code treats all four as active-low by default. Measure every wire in key-off, key-on, pre-start, running, and alarm-test conditions before relying on the mapping.

## A1-A4 analog inputs

| Input | Default use | Signal K | NMEA 2000 |
| --- | --- | --- | --- |
| A1 | Fuel tank level | `tanks.fuel.main.currentLevel` | PGN 127505 Fluid Level |
| A2 | Coolant temperature sender | `propulsion.main.coolantTemperature` | PGN 127489 engine temperature |
| A3 | Oil pressure sender | `propulsion.main.oilPressure` | PGN 127489 oil pressure |
| A4 | Alternator / charge voltage | `propulsion.main.alternatorVoltage` | PGN 127489 alternator potential |

Use passive voltage measurement when tapping an existing gauge/sender circuit. Use HALMET active/current/resistance mode only when the existing gauge is not driving the sender and the HALMET revision supports that mode for the use case.

## 1-Wire temperature sensors

Connect waterproof DS18B20-style probes to the HALMET 1-Wire header. For stable naming, enter each sensor ROM ID in `TEMPERATURE_CHANNEL_HINTS` in `include/yanmar_config.h`.

Surface-mounted DS18B20 probes are useful trend sensors, not replacements for the Yanmar overtemperature alarm or shutdown protection.

## SSD1306 OLED display

The firmware initializes a 128x64 SSD1306 OLED on the HALMET I2C bus at address `0x3c` with no reset pin. The display shows a boot page, then refreshes once per Signal K publish interval with RPM, engine state, D1-D3 warning states, SD20 seal alarm state, fuel, coolant, oil pressure, and the current Wi-Fi station or provisioning AP IP address.

If the serial log reports `SSD1306 display not found; local display disabled`, check the I2C wiring, display power, and whether the module uses address `0x3d`.

## Optional vibration sensor

The optional vibration-monitoring subsystem uses a rigidly mounted 3-axis accelerometer or IMU on the engine block or crankcase. Use the existing I2C bus for the first implementation:

| HALMET / ESP32 | Sensor breakout |
| --- | --- |
| 3.3 V | VIN or 3V3, depending on breakout |
| GND | GND |
| `GPIO21` SDA | SDA |
| `GPIO22` SCL | SCL |
| Optional verified free GPIO | INT / DRDY |

Existing I2C devices use `0x3c` for the OLED and `0x4b` for the ADS1115. Typical accelerometer addresses such as `0x68`, `0x69`, `0x53`, or `0x1d` should not conflict, but always confirm with an I2C scan.

Preferred sensor order is ICM-42688-P for a new permanent installation, ADXL345 for a simple accelerometer proof of concept, MPU-9250 if a legacy module is already available, and MPU-6050 for the cheapest I2C proof of concept. MPU-6050 and MPU-9250 are obsolete parts and should not be the preferred choice for a new permanent HALMET hardware revision.

Mount the sensor to a rigid metal point on the engine block or crankcase, preferably around the middle cylinder area below the cylinder head and above the oil pan. Do not mount it on the HALMET enclosure, loose wiring, rubber mounts, hoses, exhaust parts, alternator brackets, valve cover, or thin guards for the baseline installation.

Keep plain I2C cable runs short: target 20-30 cm, treat about 50 cm as the normal design maximum, and avoid plain I2C above about 1 m in the engine bay. For longer runs, use differential I2C, SPI where the sensor supports it, or a remote vibration node. Mount the IMU on the engine, but keep the HALMET enclosure off-engine on a nearby protected bulkhead or panel.

See [vibration monitoring](vibration.md) for sensor selection, bus choice, cable length, shielding, mounting, axis convention, wiring, long-cable options, and FFT defaults.
