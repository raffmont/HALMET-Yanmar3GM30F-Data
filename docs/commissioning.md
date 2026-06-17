# Commissioning checklist

## Bench

- [ ] Build the project with `platformio run`.
- [ ] Upload to HALMET.
- [ ] Complete SensESP Wi-Fi onboarding.
- [ ] Pair with Signal K server.
- [ ] Confirm `propulsion.main.revolutions` with a pulse generator.
- [ ] Confirm A1-A4 scaling with known voltages.
- [ ] Confirm each DS18B20 ROM ID and update `TEMPERATURE_CHANNEL_HINTS`.
- [ ] Confirm PGNs `127488`, `127489`, and `127505` on a test NMEA 2000 bus.

## On vessel, engine stopped

- [ ] Verify D1-D4 voltage levels in key-off, key-on, and alarm-test states.
- [ ] Verify `PANEL_ACTIVE_LOW`.
- [ ] Verify no factory alarm or gauge behaviour changes when HALMET is connected.
- [ ] Verify fuel sender calibration at known tank levels.

## On vessel, engine running

- [ ] Compare RPM with the Yanmar tachometer.
- [ ] Adjust `RPM_PULSES_PER_REV`.
- [ ] Compare coolant temperature and oil pressure against known-good instruments.
- [ ] Confirm alternator voltage with a multimeter.
- [ ] Confirm NMEA 2000 engine instance and tank instance on the MFD / Signal K server.
