# Vibration monitoring

Vibration monitoring is an optional HALMET diagnostic subsystem for trend comparison and engine-health observation on the Yanmar 3GM30F. It is not a shutdown system, safety interlock, or replacement for the original Yanmar alarm panel.

The intended data flow is:

```text
Engine block accelerometer / IMU
  -> ESP32 fixed-rate sampling
  -> DC removal and Hann-windowed FFT
  -> small Signal K vendor metrics
  -> reduced spectrum JSON for the Signal K plugin graph
```

Do not stream raw high-rate acceleration samples to Signal K. Publish small metrics as documented vendor extension paths and expose reduced spectrum data through the Signal K server plugin.

## Sensor choices

Preferred order:

| Sensor | Best role | Notes |
| --- | --- | --- |
| ICM-42688-P | New permanent installation | Modern IMU with FIFO, interrupts, I2C, and SPI. Use accelerometer only at first. |
| ADXL345 | Simple proof of concept | Accelerometer-only, easy to integrate, supports I2C and SPI. |
| MPU-9250 | Legacy module already available | Acceptable for software bring-up, but obsolete. Ignore gyro and magnetometer initially. |
| MPU-6050 | Cheapest I2C proof of concept | Acceptable for phase-0 or phase-1 tests, but obsolete and I2C-only. |

Treat MPU-6050 and MPU-9250 installations as proof-of-concept hardware unless the obsolete status and exact breakout-board limitations are deliberately accepted. In this HALMET project, treat MPU-6050 as I2C-only; the related MPU-6000 supports SPI, but MPU-6050 installations should be documented and wired as I2C.

## Bus choice

| Sensor | Preferred bus for first prototype | Permanent install guidance |
| --- | --- | --- |
| MPU-6050 | I2C | Keep cable short; do not design this as a long-cable sensor. |
| MPU-9250 | I2C first; SPI if the breakout exposes it | Prefer SPI over I2C if cable length or sample stability becomes a problem. |
| ADXL345 | I2C first; SPI if available | SPI is preferred for higher sampling reliability. |
| ICM-42688 | I2C first; SPI preferred for final build | Best choice for a robust permanent installation. |

Use the data-ready interrupt pin when the breakout exposes it. The interrupt allows the ESP32 to collect samples at a stable rate, which improves FFT quality compared with irregular polling.

Before installing a sensor, confirm:

- 3.3 V supply or regulator compatibility.
- 3.3 V-safe I/O logic.
- `+/-16 g` acceleration range or better.
- At least 1000 acceleration samples per second.
- I2C address does not conflict with the OLED at `0x3c` or ADS1115 at `0x4b`.
- Real mounting holes or a rigid module enclosure.
- Temperature rating suitable for the chosen engine location.
- Pullups, regulator, and level shifting are understood for the exact breakout board.

## Physical placement

Mounting stiffness matters more than the exact chip. The first baseline location should be a rigid, flat metal point on the side of the engine block or crankcase, near the middle cylinder area, below the cylinder head, and close to the crankcase or main-bearing structure.

Good alternatives are the front timing-gear cover or front block face, rear bellhousing or gearbox adapter, engine-side mount bracket before the rubber mount, or gearbox case when the diagnostic question is drivetrain-specific.

Avoid mounting on:

- HALMET electronics enclosure.
- Loose wiring, cable ties, hoses, pipes, or injection lines.
- Rubber engine mounts or the hull side of the mount.
- Exhaust manifold, mixing elbow, exhaust hose, belts, pulleys, or rotating parts.
- Alternator body, alternator adjustment arm, valve cover, thin covers, or belt guards for the first baseline.

Use a rigid metal bracket, short standoffs, strain relief, and a serviceable connector. Do not use foam tape, soft mounts, silicone blobs, or cable ties for the baseline sensor.

The IMU must measure engine vibration, not hull vibration and not cable movement. Mount the IMU on the engine, but keep the HALMET enclosure off-engine on a nearby bulkhead, panel, or other fixed structure protected from heat, oil, spray, and direct engine vibration. Do not mount the HALMET box on the engine merely to shorten the cable.

Recommended first installation on a Yanmar 3GM30F:

```text
IMU:    bolted to a clean, flat point on the engine block or a rigid block-mounted bracket.
HALMET: mounted off-engine on a nearby bulkhead or protected panel.
Cable:  20-30 cm shielded/twisted cable with a small service loop.
```

Recommended axis convention:

| Axis | Direction |
| --- | --- |
| X | Fore-aft, parallel to crankshaft / boat centerline |
| Y | Port-starboard / athwartship |
| Z | Vertical |

Record the actual sensor orientation and mount location during commissioning. Do not apply rotation correction in the first implementation; publish per-axis or primary-axis metrics and document the installation.

## Wiring

The first implementation should use the existing HALMET I2C bus:

| HALMET / ESP32 | Sensor breakout |
| --- | --- |
| 3.3 V | VIN or 3V3, depending on breakout |
| GND | GND |
| `GPIO21` SDA | SDA |
| `GPIO22` SCL | SCL |
| Optional verified free GPIO | INT / DRDY |

## IMU-to-HALMET cable length

The vibration sensor shall be mounted on the engine, while the HALMET enclosure should remain on a nearby bulkhead, panel, or other fixed structure protected from heat, oil, spray, and direct engine vibration.

For I2C sensors such as MPU-6050, MPU-9250, ADXL345, or ICM-42688 breakouts, keep the cable short:

| Cable run | Plain I2C recommendation |
| ---: | --- |
| 10-30 cm | Recommended target for engine vibration sensing. |
| Up to about 50 cm | Acceptable if wiring is shielded/twisted, routed carefully, and tested on board. |
| Around 1 m | Risky on plain I2C in an engine bay; use only after testing at the chosen bus speed. |
| More than 1 m | Avoid plain I2C. Use differential I2C, SPI where appropriate, or a remote sensor node. |

The practical limit is not a fixed length. It depends on cable capacitance, pull-up value, bus speed, electrical noise, grounding, and connector quality. Start with 100 kHz I2C for reliability. Increase to 400 kHz only after confirming stable samples and no I2C errors on board.

For this project, use this design rule:

```text
Target I2C cable length:       20-30 cm
Maximum plain-I2C design goal: 50 cm
Use an extender or remote node above: 1 m
```

## IMU cable wiring

Recommended I2C wiring:

```text
HALMET 3V3  -> IMU VCC
HALMET GND  -> IMU GND
HALMET SDA  -> IMU SDA
HALMET SCL  -> IMU SCL
ESP32 GPIO  -> IMU INT / data-ready, optional but recommended
```

Use these installation rules:

- Use a short shielded cable or twisted-pair cable.
- Prefer one twisted pair for SDA/GND and one twisted pair for SCL/GND.
- Keep the cable away from the alternator output cable, starter cable, injector wiring, glow-plug wiring, and high-current battery wiring.
- Connect the shield to HALMET/device ground at one end only unless the final marine grounding design requires otherwise.
- Add strain relief at both the engine end and the HALMET enclosure end.
- Leave a small service loop so engine movement does not pull on the sensor or connector.
- Add local decoupling near the IMU breakout: at least 100 nF ceramic, plus 4.7 uF to 10 uF if the breakout does not already provide bulk capacitance.
- Use 3.3 V logic. Do not pull SDA/SCL up to 5 V.
- Start with 2.2 kOhm to 4.7 kOhm pull-ups to 3.3 V, then adjust only if bus rise time or reliability requires it.
- Start I2C at 100 kHz. Use 400 kHz only after testing.

For future high-rate or longer-cable installations, prefer SPI when the selected sensor supports it. The current HALMET mapping already uses common ESP32 VSPI pins for other functions, so SPI requires explicit board-revision review before assigning pins.

## Long cable options

Do not use plain I2C directly for long engine-bay cable runs when the IMU must be farther than 1 m from HALMET.

Use one of these approaches:

1. Differential I2C extender

   Use a differential I2C buffer such as PCA9615 or an equivalent circuit. Place one transceiver close to HALMET and the other close to the IMU. Run the differential pairs through shielded twisted cable.

2. SPI, only for sensors and breakouts that expose SPI

   SPI can be more robust for deterministic high-rate sampling than plain I2C, but it is still not a magic long-distance bus. Keep the cable short, route it carefully, and validate the signal on board. Use SPI only with sensors that actually support it, such as MPU-9250, ADXL345, or ICM-42688 breakouts. Do not configure MPU-6050 as SPI.

3. Remote vibration node

   For several meters, put a small microcontroller close to the IMU, sample the IMU locally, compute RMS/FFT locally, and send only the vibration metrics or reduced spectrum to HALMET or directly to Signal K over a more suitable link.

## Sampling and FFT defaults

Recommended phase-1 defaults:

| Parameter | Value |
| --- | ---: |
| Sample rate | 1000 Hz |
| FFT size | 1024 samples |
| Window | Hann |
| Useful bandwidth | 0-500 Hz |
| Reduced graph bandwidth | 0-250 Hz |
| Bin width | 0.9765625 Hz |
| Spectrum update | About once per second |
| Acceleration unit | m/s2 |

For each FFT window, remove DC and gravity by subtracting the axis mean, compute RMS acceleration, select a primary FFT axis, find the dominant peak above about 2 Hz, and compute order amplitudes from current RPM:

```text
crank_frequency_hz = rpm / 60
firing_frequency_hz = 1.5 * rpm / 60
```

For a 3-cylinder 4-stroke engine, the firing frequency examples are:

| RPM | Crank Hz | Firing Hz |
| ---: | ---: | ---: |
| 1200 | 20 | 30 |
| 1800 | 30 | 45 |
| 2400 | 40 | 60 |
| 3000 | 50 | 75 |

## Signal K data model

Small metrics use HALMET vendor extension paths:

| Metric | Signal K path | Units |
| --- | --- | ---: |
| RMS acceleration | `halmet.yanmar3gm30f.vibration.accelerationRms` | m/s2 |
| Peak acceleration | `halmet.yanmar3gm30f.vibration.peakAcceleration` | m/s2 |
| Dominant frequency | `halmet.yanmar3gm30f.vibration.peakFrequency` | Hz |
| Crank-order amplitude | `halmet.yanmar3gm30f.vibration.crankOrderAmplitude` | m/s2 |
| Firing-order amplitude | `halmet.yanmar3gm30f.vibration.firingOrderAmplitude` | m/s2 |
| Sample rate | `halmet.yanmar3gm30f.vibration.sampleRate` | Hz |
| FFT size | `halmet.yanmar3gm30f.vibration.fftSize` | count |
| Status | `halmet.yanmar3gm30f.vibration.status` | string |
| Reduced spectrum JSON | `halmet.yanmar3gm30f.vibration.spectrum` | JSON |

These are diagnostics, not standard Signal K propulsion schema paths. Do not publish `propulsion.main.vibration.*` unless Signal K later standardizes such a branch.

The reduced spectrum JSON should be rate-limited, defaulting to about one update per second:

```json
{
  "timestampMs": 1234567,
  "axis": "z",
  "sampleRateHz": 1000,
  "fftSize": 1024,
  "binWidthHz": 0.9765625,
  "maxFrequencyHz": 250,
  "magnitudesMps2": [0.01, 0.02, 0.03]
}
```

The Signal K server plugin exposes the latest metrics and reduced spectrum at:

```text
/plugins/signalk-halmet-yanmar3gm30f-data/vibration
/plugins/signalk-halmet-yanmar3gm30f-data/spectrum
```

The `/spectrum` endpoint is an alias for graph clients.

## Commissioning checklist

Before enabling FFT/spectrum publishing:

- Confirm the IMU is rigidly attached to the engine.
- Confirm the HALMET enclosure is not mounted directly on the engine.
- Confirm the IMU cable is strain-relieved at both ends.
- Confirm the cable is not routed alongside starter, alternator, glow-plug, or battery cables.
- Confirm SDA/SCL pull-ups go to 3.3 V, not 5 V.
- Confirm I2C scan sees the IMU address and does not conflict with existing devices.
- Confirm stable sampling at the selected sample rate while the engine is off.
- Confirm stable sampling at idle, cruise RPM, and alternator load.
- Confirm the reported dominant frequencies make sense relative to RPM.
- Record a baseline spectrum after installation.

## References

- NXP UM10204, I2C-bus specification and user manual: <https://www.nxp.com/docs/en/user-guide/UM10204.pdf>
- NXP PCA9615 differential I2C buffer data sheet: <https://www.nxp.com/docs/en/data-sheet/PCA9615.pdf>
