# ESPHome Precision Plex LIN

ESPHome external component and reference firmware for monitoring a Precision
Plex motorhome LIN network with an ESP32 and LIN transceiver.

The bridge emits a compact, versioned Home Assistant event snapshot used by
the `precision_plex` integration. That integration owns the user-facing
entities, prefers fresh LIN values, and
falls back to the factory Wireless TP Bluetooth connection when appropriate.
Commands remain on Bluetooth while the safe LIN command lifecycle is still
being characterized.

## Current release

**v0.6.1**, based on field-tested LIN Analyzer **Build 013.1**.

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

Full generator runtime decoding remains an open investigation.

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
      ref: v0.6.1
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
command injection is intentionally not exposed. Release and explicit-stop
behavior must be captured and validated first.

## Repository layout

See [Repository Layout](docs/repository_layout.md).
