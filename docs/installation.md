# Installation

## Requirements

- ESPHome 2026.6.4 or newer
- ESP32 DevKit-compatible board
- Automotive LIN transceiver suitable for the coach electrical environment
- Common ground between the ESP32 supply, LIN transceiver, and coach
- Home Assistant with the ESPHome integration
- Precision Plex integration v5.5.15 or newer for restart-safe snapshots

## Configure the node

1. Copy `esphome/examples/precision_plex_lin_analyzer.yaml` into the ESPHome
   configuration directory.
2. Preserve the node name `esphome-web-055454` when upgrading the field-tested
   bridge. A new installation may choose another stable node name.
3. Create `secrets.yaml` using `secrets.example.yaml` as the template.
4. Review the UART and GPIO25 wiring before compiling. On ESP32, use
   `rx_full_threshold: 1` on the LIN UART for the direct PID5E awning-light
   test. This lets the firmware see the poll header as each byte arrives.

The example pins the external component to `v0.6.9`. Keep production systems
on a release tag instead of `main` so later repository changes cannot silently
alter a known-good build.

## Validate and install

Run ESPHome validation, compile, and install through the normal ESPHome UI or
CLI. After boot, verify Home Assistant's Precision Plex `Telemetry Transport`
reports `lin`, its `lin_event_snapshot_active` attribute is `true`, and the
new LIN-only HVAC, tank-heater, AC/converter, and ignition entities become
available as their source PIDs are observed.

The Home Assistant integration uses the example's native ESPHome actions for
desired-state output commands and leased patio-awning and slide movement. Enable the
bridge to perform Home Assistant actions so it can publish feedback events.

## Generator control

The v0.6.8 example includes the native generator action used by Precision
Plex integration v5.5.19. When upgrading an existing configuration, add the
action below to the ESPHome `api.actions` list alongside the existing output
and motion actions.

```yaml
- action: precision_plex_generator_command
  variables:
    command_key: string
    request_id: string
  then:
    - lambda: id(lin_bridge).generator_command(command_key, request_id);
```

The bridge uses fresh PIDBA status for command acceptance. A missing
acknowledgement or incomplete burst does not permit a Bluetooth retry.

## Updating

1. Change the external-component `ref` to the new release tag.
2. Review the changelog for hardware, entity-contract, or integration changes.
3. Validate and install the ESPHome node.
4. Update the Precision Plex Home Assistant integration when the release notes
   specify a paired version.
5. Verify LIN telemetry, stale-source fallback, and one known movement state.
