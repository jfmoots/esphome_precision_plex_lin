# ESPHome Precision Plex LIN v0.6.2 — Complete Generator Runtime Telemetry

This release completes the LIN-side generator telemetry used by the Precision
Plex Home Assistant integration.

## Generator runtime

- Decodes PIDBA data bytes 1-3 as a little-endian packed-BCD whole-hour
  counter and combines them with the page low-nibble tenths digit.
- Publishes the complete runtime in hours and tenths through the snapshot event.
- Validates every BCD digit before publishing the value.
- The observed LIN frame decodes to `125.4` hours and exactly matches Bluetooth.
- The future `125.9 -> 126.0` transition remains the final field check of the
  whole-hour rollover.

## Generator running correction

- Corrects the PID32 Generator Running bit location to data byte 6 bit `0x40`.
- Retains PIDBA generator state as an additional running-state source.

Pair with Precision Plex integration v5.5.5.
