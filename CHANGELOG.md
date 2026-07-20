# Changelog

## v0.6.0 - Integration-Owned Event Telemetry

- Adds a versioned `on_snapshot` component trigger and one-second freshness
  heartbeat for the Precision Plex integration.
- Moves decoded entity naming, availability, and transport policy out of the
  production ESPHome YAML and into Home Assistant.
- Includes PIDBA core telemetry, PID32 outputs/movement, PIDEC coach flags,
  and independently fresh PID37 HVAC zones in the snapshot contract.
- Reduces the production node from dozens of template entities to the LIN
  component plus its two hardware/research buttons.
- Retains Build 013.1 movement decodes and Bluetooth command ownership.

## v0.5.1 - External Component Compile Fix

- Makes every top-level decoder header safe for ESPHome's generated aggregate
  header and removes the recursive `esphome.h` include from protocol primitives.
- Replaces the broad recursive include with targeted logging and UART headers.
- Adds the required targeted UART component include.
- Renames the ESPHome wrapper namespace to avoid colliding with the legacy
  decoder namespace.
- Fixes LIN TX counter log formatting for current ESP32 toolchains.
- Leaves the Build 013.1 telemetry behavior and Home Assistant entity contract
  unchanged.

## v0.5.0 - GitHub External Component Baseline

- Converts the field-tested LIN Analyzer Build 013.1 into a versioned ESPHome
  external component.
- Preserves automatic LIN parsing, telemetry entities, freshness signals,
  flight recording, and the safe PID5E awning-light command test.
- Includes the corrected PID32 direction mappings for the bedroom slide and
  sofa extend.
- Adds a production example pinned to the `v0.5.0` GitHub release tag.
- Records the observed PID5E motor-command sequence while leaving motor control
  disabled pending stop/release captures.

The full legacy build history is retained in
[`docs/protocol/CHANGELOG.md`](docs/protocol/CHANGELOG.md).
