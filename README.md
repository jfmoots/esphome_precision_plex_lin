# ESPHome Precision Plex LIN

ESPHome external component and reference firmware for monitoring a Precision
Plex motorhome LIN network with an ESP32 and LIN transceiver.

The bridge emits a compact, versioned Home Assistant event snapshot used by
the `precision_plex` integration. That integration owns the user-facing
entities, prefers fresh LIN values, and
falls back to the factory Wireless TP Bluetooth connection when appropriate.
Commands remain on Bluetooth while the bridge observes the fast LIN command
channels for immediate requested-state feedback.

## Current release

**v0.6.3**, based on field-tested LIN Analyzer **Build 013.1**.

Current telemetry includes:

- House battery voltage
- Fresh, grey, and black tanks
- Propane level
- Generator state and running status
- Water pump, water heater, tank heater, and awning-light states
- Patio awning, sofa, bedroom slide, and wardrobe slide movement directions
- Two HVAC-zone decoders
- AC/converter and ignition flags
- Independent per-source freshness in the event contract
- Normalized PID1F/PID5E command-intent events

PIDBA now publishes the complete generator runtime using validated packed BCD.
The observed `125.4` LIN value matches Bluetooth; the next whole-hour rollover
will provide final field confirmation of the carry behavior.

## Installation

Start with the complete production example:

```text
esphome/examples/precision_plex_lin_analyzer.yaml
```

It loads the component from GitHub using a pinned release tag:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/jfmoots/esphome_precision_plex_lin
      ref: v0.6.3
      path: esphome/components
    components:
      - precision_plex_lin
    refresh: 0s
```

Copy `esphome/examples/secrets.example.yaml` to `secrets.yaml`, enter the Wi-Fi
credentials, validate, and install from ESPHome.

The production YAML intentionally creates no decoded telemetry entities. It
keeps only the flight-recorder and guarded awning-light research buttons; the
Precision Plex integration creates all coach telemetry entities.

See [Installation](docs/installation.md) and [Hardware](docs/hardware.md) for
the complete procedure and wiring notes.

## Safety and command status

Normal operation is telemetry-focused. The repository retains one proven,
low-risk diagnostic transmit action: the muted PID5E awning-light toggle.

Motor command payloads have been observed and documented, but slide and awning
command injection is intentionally not exposed. The captured release behavior
now supports state observation, but a safe public motor-injection lifecycle is
still deliberately out of scope.

The bridge does listen to valid PID1F touchscreen and PID5E Wireless TP
commands. It collapses the request/active pair for each motion into one start,
ignores repeated holds and housekeeping, and publishes the matching release as
a versioned event. This is observation only; it does not bypass factory
interlocks or add motor injection controls.

## Repository layout

See [Repository Layout](docs/repository_layout.md).
