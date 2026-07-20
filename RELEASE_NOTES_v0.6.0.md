# ESPHome Precision Plex LIN v0.6.0 — Event Telemetry Transport

This release pairs with Precision Plex Home Assistant integration v5.5.0.

## Changed

- Emits one compact, versioned LIN snapshot event per second instead of
  creating and republishing dozens of ESPHome template entities.
- Reports independent freshness for PIDBA core telemetry, PID32 outputs,
  PIDEC coach flags, and each PID37 HVAC zone.
- Moves entity names, availability, source selection, and user-facing
  diagnostics into the Precision Plex integration.
- Reduces the production YAML to the decoder/transport and two guarded
  hardware-research buttons.

## Preserved

- Field-tested Build 013.1 movement-direction decodes.
- LIN flight recording and the guarded PID5E awning-light test.
- Bluetooth ownership of all normal commands.

Full generator runtime decoding remains an open LIN investigation.
