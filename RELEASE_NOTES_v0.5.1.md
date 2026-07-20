# ESPHome Precision Plex LIN v0.5.1

This maintenance release fixes the initial GitHub external-component build on
current ESPHome/ESP-IDF toolchains.

## Fixed

- Makes the decoder headers safe when ESPHome includes them individually in
  its generated aggregate header.
- Removes the recursive `esphome.h` include that caused LIN types such as
  `Frame` and UART declarations to be unavailable during compilation.
- Eliminates the namespace collision between the ESPHome component wrapper and
  the legacy LIN decoder implementation.
- Adds the required targeted UART and logging includes.

## Unchanged

- All Build 013.1 LIN decodes and movement corrections.
- Existing ESPHome node name, entity names, and entity IDs.
- Precision Plex v5.4.2 LIN telemetry contract.
- Bluetooth command behavior and the guarded LIN command-research path.

Update the YAML external-component reference from `v0.5.0` to `v0.5.1`, clean
the ESPHome build files, and compile again.
