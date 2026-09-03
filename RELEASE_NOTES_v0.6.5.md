# ESPHome Precision Plex LIN v0.6.5 — Restart-Safe Snapshot Recovery

This release restores complete LIN telemetry after Home Assistant restarts
while the ESP32 bridge remains online.

- Sends a complete snapshot on bridge startup and every 30 seconds.
- Retains compact two-second heartbeats between complete snapshots.
- Adds `command_age_ms` so a cached command is not mistaken for a new command
  when Home Assistant receives its first complete snapshot.

Pairs with Precision Plex integration v5.5.15.
