# Architecture

```text
Precision Plex LIN bus
        |
        v
ESP32 UART + LIN transceiver
        |
        v
precision_plex_lin external component
        |-- frame synchronization and checksum validation
        |-- PID decoders and freshness tracking
        |-- normalized PID1F/PID5E command-intent observer
        |-- protocol learners and flight recorder
        `-- guarded PID5E diagnostic transmit path
        |
        v
versioned Home Assistant event snapshot
        |
        v
Precision Plex Home Assistant integration
        `-- LIN preference with field-by-field BLE fallback
```

The component owns LIN timing, parsing, decoded state, and command-path
observation. PID1F and PID5E each retain their own active-motion context; the
component emits only meaningful toggle, start, and release edges. PID32 stays
authoritative. The example YAML publishes the snapshot event contract. The Home
Assistant integration owns the user-facing lights, switches, covers, sensors,
transport selection, and Bluetooth command path.
