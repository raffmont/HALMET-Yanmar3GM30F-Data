# Signal K model inventory

This directory contains firmware-facing Signal K path inventory and sample mapping files. It is not a Signal K server plugin.

The server plugin and web GUI live in `signalk/`.

`paths.json` includes both standard vessel schema paths and documented HALMET vendor extension diagnostics. Optional vibration paths are kept under `halmet.yanmar3gm30f.vibration.*` so firmware and plugin code do not create non-schema children under `propulsion.main`.
