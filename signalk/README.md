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

## Installation for development

Copy this directory into the Signal K server configuration `node_modules` directory, or install it from this directory with npm in the Signal K server home directory.

```bash
npm install /path/to/HALMET-Yanmar3GM30F-Data/signalk
```

Restart Signal K server, enable the plugin in the admin UI, then open the web application from the Signal K app list.
