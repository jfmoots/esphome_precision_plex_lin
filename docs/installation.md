# Installation

## Requirements

- ESPHome 2026.6.4 or newer
- ESP32 DevKit-compatible board
- Automotive LIN transceiver suitable for the coach electrical environment
- Common ground between the ESP32 supply, LIN transceiver, and coach
- Home Assistant with the ESPHome integration
- Precision Plex integration v5.4.2 or newer for LIN-preferred telemetry

## Configure the node

1. Copy `esphome/examples/precision_plex_lin_analyzer.yaml` into the ESPHome
   configuration directory.
2. Preserve the node name `esphome-web-055454` when upgrading the field-tested
   bridge. A new installation may choose another stable node name.
3. Create `secrets.yaml` using `secrets.example.yaml` as the template.
4. Review the UART and GPIO25 wiring before compiling.

The example pins the external component to `v0.5.1`. Keep production systems
on a release tag instead of `main` so later repository changes cannot silently
alter a known-good build.

## Validate and install

Run ESPHome validation, compile, and install through the normal ESPHome UI or
CLI. After boot, verify:

- `LIN Firmware Version` reports the expected release/build.
- `LIN Bus Active` becomes on.
- `LIN Telemetry Active` and `LIN Output State Active` become on when their
  source PIDs are fresh.
- Home Assistant's Precision Plex `Telemetry Transport` reports `lin`.

## Updating

1. Change the external-component `ref` to the new release tag.
2. Review the changelog for hardware, entity-contract, or integration changes.
3. Validate and install the ESPHome node.
4. Update the Precision Plex Home Assistant integration when the release notes
   specify a paired version.
5. Verify LIN telemetry, stale-source fallback, and one known movement state.
