# ESPHome Precision Plex LIN v0.6.4 — Compact Heartbeat Snapshots

This release keeps rapid LIN bridge-failure detection while sharply reducing
the size of unchanged heartbeat events sent to Home Assistant.

## Compact liveness events

- Keeps the established two-second heartbeat and four-second integration
  timeout.
- Sends only schema, firmware version, sequence, uptime, bus health, source
  freshness, and command-intent capability when telemetry is unchanged.
- Sends the complete versioned snapshot immediately whenever decoded telemetry
  or normalized command intent changes.

## Compatibility

- Snapshot schema remains version 1.
- Precision Plex integration v5.5.8 merges compact heartbeats into the last
  complete snapshot.
- Older integrations should remain on firmware v0.6.3 until upgraded.
