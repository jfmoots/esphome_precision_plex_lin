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
        |-- PID1F/PID5E learners and flight recorder
        `-- guarded PID5E diagnostic transmit path
        |
        v
ESPHome telemetry entities
        |
        v
Precision Plex Home Assistant integration
        `-- LIN preference with field-by-field BLE fallback
```

The component owns LIN timing, parsing, decoded state, and command-path
research. The example YAML publishes the stable ESPHome entity contract. The
Home Assistant integration owns the user-facing lights, switches, covers,
sensors, transport selection, and Bluetooth command fallback.
