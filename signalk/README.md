# Signal K server plugin

This directory contains a Signal K server plugin and web application for the HALMET Yanmar 3GM30F data firmware.

## Contents

- `package.json`: plugin metadata for installation in the Signal K server application directory.
- `plugin/index.js`: server-side plugin code.
- `public/`: web GUI mounted by Signal K server at `/signalk-halmet-yanmar3gm30f-data`.
- `assets/`: application icon and static assets.

## Features

- Displays live engine RPM, engine state, fuel level, fuel remaining, coolant temperature, oil pressure, alternator voltage, and engine-room temperature.
- Generates `tanks.fuel.<id>.remaining` from `tanks.fuel.<id>.currentLevel` and the configured tank capacity.
- Generates `propulsion.<id>.state` from `propulsion.<id>.revolutions` when the firmware has not already supplied a state update.
- Exposes diagnostic JSON endpoints at `/plugins/signalk-halmet-yanmar3gm30f-data/status` and `/plugins/signalk-halmet-yanmar3gm30f-data/diagnostics`.
- Exposes optional vibration metrics and reduced FFT spectrum JSON at `/plugins/signalk-halmet-yanmar3gm30f-data/vibration` and `/plugins/signalk-halmet-yanmar3gm30f-data/spectrum`.
- Displays the latest vibration status, RMS acceleration, dominant frequency, engine-order amplitudes, and spectrum graph when the firmware publishes `halmet.yanmar3gm30f.vibration.*` paths.

## Vibration data

The plugin does not create Signal K vibration paths. It reads documented HALMET vendor extension diagnostics from the server data tree:

```text
halmet.yanmar3gm30f.vibration.accelerationRms
halmet.yanmar3gm30f.vibration.peakAcceleration
halmet.yanmar3gm30f.vibration.peakFrequency
halmet.yanmar3gm30f.vibration.crankOrderAmplitude
halmet.yanmar3gm30f.vibration.firingOrderAmplitude
halmet.yanmar3gm30f.vibration.sampleRate
halmet.yanmar3gm30f.vibration.fftSize
halmet.yanmar3gm30f.vibration.status
halmet.yanmar3gm30f.vibration.spectrum
```

The `spectrum` value may be a JSON object or JSON string containing reduced FFT data such as `sampleRateHz`, `fftSize`, `binWidthHz`, `maxFrequencyHz`, `axis`, `magnitudesMps2`, optional `frequenciesHz`, and optional `topPeaks`.

Vibration monitoring is for diagnostics and trend comparison only. It must not be used for automatic engine shutdown or replacement of the Yanmar alarm system.

## Installation for development

Copy this directory into the Signal K server configuration `node_modules` directory, or install it from this directory with npm in the Signal K server home directory.

```bash
npm install /path/to/HALMET-Yanmar3GM30F-Data/signalk
```

Restart Signal K server, enable the plugin in the admin UI, then open the web application from the Signal K app list.
