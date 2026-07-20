# Installation

## Requirements

- ESPHome 2026.6.4 or newer
- ESP32 DevKit-compatible board
- Automotive LIN transceiver suitable for the coach electrical environment
- Common ground between the ESP32 supply, LIN transceiver, and coach
- Home Assistant with the ESPHome integration
- Precision Plex integration v5.5.1 or newer for change-driven event snapshots

## Configure the node

1. Copy `esphome/examples/precision_plex_lin_analyzer.yaml` into the ESPHome
   configuration directory.
2. Preserve the node name `esphome-web-055454` when upgrading the field-tested
   bridge. A new installation may choose another stable node name.
3. Create `secrets.yaml` using `secrets.example.yaml` as the template.
4. Review the UART and GPIO25 wiring before compiling.

The example pins the external component to `v0.6.1`. Keep production systems
on a release tag instead of `main` so later repository changes cannot silently
alter a known-good build.

## Validate and install

Run ESPHome validation, compile, and install through the normal ESPHome UI or
CLI. After boot, verify Home Assistant's Precision Plex `Telemetry Transport`
reports `lin`, its `lin_event_snapshot_active` attribute is `true`, and the
new LIN-only HVAC, tank-heater, AC/converter, and ignition entities become
available as their source PIDs are observed.

## Updating

1. Change the external-component `ref` to the new release tag.
2. Review the changelog for hardware, entity-contract, or integration changes.
3. Validate and install the ESPHome node.
4. Update the Precision Plex Home Assistant integration when the release notes
   specify a paired version.
5. Verify LIN telemetry, stale-source fallback, and one known movement state.
