# ESPHome Precision Plex LIN v0.6.3 — Unified Command Intent Observer

This release adds a low-overhead, edge-driven observer for the two fast command
channels already present on the Precision Plex LIN bus.

## One event contract for PID1F and PID5E

- Observes both the touchscreen PID1F slot and Wireless TP PID5E slot.
- Publishes normalized toggle, motion-start, and motion-stop events in the
  existing versioned Home Assistant snapshot.
- Uses identical entity keys for both command sources, so the integration does
  not need a separate state machine per controller.

## Traffic normalization

- Collapses each lower motion request opcode and its `+0x40` active opcode into
  one logical start.
- Ignores repeated active/hold frames.
- Emits `3F 00` as a stop only when the same source channel had active motion.
- Ignores PID5E `B0 02` and `B1 01` housekeeping.
- Feeds locally injected PID5E commands into the observer once; a matching bus
  echo is naturally deduplicated.

## Compatibility and authority

- PID32 remains the authoritative output bitmap.
- Snapshot schema remains version 1 with optional command-intent fields, so
  existing integrations continue to receive telemetry.
- Pair with Precision Plex Home Assistant integration v5.5.6.
