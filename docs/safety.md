# Safety notes

This project monitors an inboard marine diesel engine. It must not replace factory alarms, fuses, breakers, shutdown procedures, engine-panel lamps, or marine-grade wiring practice.

- Bench-test every input before connecting to the vessel harness.
- Keep all modifications reversible and documented.
- Fuse any added supply wire close to the source.
- Use strain relief and mechanically secure all taps.
- Do not share grounds across an isolation boundary unless the board/module documentation requires it.
- Treat surface-mounted temperature probes as trend sensors only.
- Confirm that NMEA 2000 output does not conflict with an existing engine gateway using the same instances.
