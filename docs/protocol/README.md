# Precision Plex LIN Analyzer Documentation

This folder captures the current reverse-engineered LIN protocol knowledge for the Precision Plex coach network.

Build 012.0 marks the transition from a pure analyzer toward the foundation of a wired Precision Plex interface:

- PID1F remains the touchscreen command channel and command learner.
- PID5E is the Wireless TP / BLE bridge command slot.
- The ESP can claim the Wireless TP PID5E slot using the MCP2004E FAULT/TXE line.
- The PMM accepts injected PID5E responses when the Wireless TP is claimed.
- The 10-second flight recorder remains available for command discovery.

## Current status

| Area | Status | Notes |
|---|---|---|
| PID1F | Command learner active | Touchscreen command channel. Timing-race experiments are retired. |
| PID5E | Command injection proven | Wireless TP slot can be claimed with GPIO25 / FAULT-TXE. |
| PID32 | Mostly decoded | Output bitmap for lights, water heater, tank heater, pump, slides, awning, generator running. |
| PID37 | Mostly decoded | HVAC mode, operating state, fan/request context, room/setpoint telemetry. |
| PIDBA | Mostly decoded | Battery, tanks, propane, generator telemetry. |
| PID76 / PID9C / PIDDD | Open | Active unknown/housekeeping candidates. |
| PIDEC | Decoded flags | AC/converter present, ignition, related coach flags. |

## Hardware claim line

Wireless TP board:

- LIN transceiver: MCP2004E.
- Claim/mute point: FAULT/TXE test pad near the C10 silkscreen.
- Wiring: ESP GPIO25 -> 1k resistor -> FAULT/TXE test pad, with common ground.
- Firmware uses open-drain style control: drive low to claim, float/release for normal Wireless TP operation.

## Important finding

The PMM is not the limiting factor. It accepts and executes injected PID5E commands. Some features still have owner-specific behavior elsewhere on the bus. Tank heater is the known example: the touchscreen updates when the PMM reports the heater on, then reasserts its own configured off state, likely because it owns the battery-voltage policy.
