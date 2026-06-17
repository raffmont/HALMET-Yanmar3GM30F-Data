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
