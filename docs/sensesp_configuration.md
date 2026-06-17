# SensESP configuration

SensESP handles Wi-Fi onboarding, server discovery, Signal K websocket connection, authentication, OTA support, and the cooperative event loop.

## Device identity

Configured in `include/yanmar_config.h`:

- `HOSTNAME`: `halmet-yanmar3gm30f-data`
- `APP_NAME`: `HALMET Yanmar 3GM30F Data`
- `MODEL_ID`: `HALMET-Yanmar3GM30F-Data`
- `SOFTWARE_VERSION`: firmware version string

## Configuration panel

The firmware registers the Signal K output paths below as SensESP configuration
items. They appear in the device web UI under the Signal K/SensESP
configuration panel and can be edited without recompiling:

```text
/Yanmar3GM30F/RPM/Revolutions
/Yanmar3GM30F/Engine/State
/Yanmar3GM30F/Analog/A1 Fuel Level
/Yanmar3GM30F/Analog/A2 Coolant Temperature
/Yanmar3GM30F/Analog/A3 Oil Pressure
/Yanmar3GM30F/Analog/A4 Alternator Voltage
/Yanmar3GM30F/1Wire/<Signal K path>
```

Calibration constants remain in `include/yanmar_config.h`. A future step can
promote selected calibration values to persistent SensESP configurable items.
