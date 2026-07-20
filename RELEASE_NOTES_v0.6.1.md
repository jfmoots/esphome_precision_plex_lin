# ESPHome Precision Plex LIN v0.6.1 — Change-Driven Snapshot Delivery

This release pairs with Precision Plex integration v5.5.1 to reduce LIN
telemetry latency and eliminate redundant Home Assistant updates.

## Changed

- Sends an immediate snapshot only when decoded PID32, PIDBA, PIDEC, or PID37
  telemetry changes meaningfully.
- Keeps a two-second heartbeat for source freshness without treating repeated
  LIN frames as state changes.
- Adds `snapshot_sequence` and `snapshot_reason` transport diagnostics.

The observed PID5E-command-to-PID32-change time is approximately 43 ms in the
field captures. This change forwards that decoded result immediately instead
of waiting for the next periodic snapshot.
