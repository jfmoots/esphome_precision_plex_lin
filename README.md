# ESPHome Precision Plex LIN

ESPHome external component and reference firmware for monitoring a Precision
Plex motorhome LIN network with an ESP32 and LIN transceiver.

The bridge emits versioned events to the `precision_plex` Home Assistant
integration. The integration creates the coach entities and prefers fresh LIN
telemetry. Supported output, awning, slide, and generator commands can also
use LIN, with Bluetooth fallback when the LIN path cannot send the command.

## Current release

**v0.6.9** filters immediate telemetry snapshots by the fields published to
Home Assistant. Raw frame changes remain visible in the analyzer. The bridge
still sends two-second liveness heartbeats, periodic full snapshots, and
immediate command feedback.

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
      ref: v0.6.9
      path: esphome/components
    components:
      - precision_plex_lin
    refresh: 0s
```

Copy `esphome/examples/secrets.example.yaml` to `secrets.yaml`, enter the Wi-Fi
credentials, validate, and install from ESPHome.

The production YAML exposes native command actions and diagnostic buttons.
The Precision Plex integration creates the decoded coach telemetry entities.

See [Installation](docs/installation.md) and [Hardware](docs/hardware.md) for
the complete procedure and wiring notes.

## Safety and command status

The bridge confirms output changes using fresh PID32 state and acknowledges
generator commands using PIDBA. Awning and slide actions use a renewable
lease, local checks for safe coach state, and PID32 feedback for movement and
release. If a command may have been sent, Home Assistant reports an
unconfirmed result and ends the request. Use the factory controls to verify
coach state after an unconfirmed motion stop.

PID1F touchscreen and PID5E Wireless TP command observation continues to
provide immediate feedback about requests from the factory controls.

## Repository layout

See [Repository Layout](docs/repository_layout.md).
