# PID5E Wireless TP / BLE Bridge

PID5E is the Wireless TP response slot. The PMM polls PID5E and the Wireless TP normally answers with idle/housekeeping or a queued BLE command.

## Confirmed observations

| Payload | Meaning |
|---|---|
| `3F 00 62` | Idle / bridge ready |
| `B0 02 EE` | Wireless TP housekeeping pulse |
| `B1 01 EE` | Wireless TP housekeeping pulse |
| `00 00 A1` | Awning-light toggle command |

## Observed motion command sequence

Dedicated BLE movement captures on 2026-07-20 showed a consistent two-phase
PID5E sequence. Each movement began with one lower opcode and was followed by
the corresponding opcode plus `0x40`, repeated while the movement remained
active.

| Movement | Initial observed payload | Repeated active payload |
|---|---|---|
| Patio awning retract | `09 00` | `49 00` |
| Patio awning extend | `0A 00` | `4A 00` |
| Sofa retract | `0F 00` | `4F 00` |
| Sofa extend | `10 00` | `50 00` |
| Wardrobe slide retract | `11 00` | `51 00` |
| Wardrobe slide extend | `12 00` | `52 00` |
| Bedroom slide retract | `13 00` | `53 00` |
| Bedroom slide extend | `14 00` | `54 00` |

These captures establish the observed command payloads but do not yet establish
the complete safe injection lifecycle for motor controls. Release and explicit
stop behavior still need dedicated captures before movement command injection
is implemented.

## Proven claim/injection path

The Wireless TP uses an MCP2004E LIN transceiver. Pulling its FAULT/TXE test pad low temporarily disables the Wireless TP LIN transmitter without powering down the module.

Build 011.0 proved the following sequence:

1. Queue a PID5E command.
2. Pull FAULT/TXE low from ESP GPIO25 through a 1k resistor.
3. Wait for the PMM PID5E header `00 55 5E`.
4. Send the command payload and checksum immediately.
5. Release FAULT/TXE so the Wireless TP resumes normal operation.

The awning-light command works repeatedly using PID5E payload `00 00` and checksum `A1`. The Wireless TP/BLE entities resynchronize immediately after release.

## Current safe test

Build 012.0 keeps only the safe awning-light test button:

```text
LIN Test PID5E Muted Awning Light Toggle
```

This remains the preferred command-path sanity test.

## Tank-heater finding

Payload `06 00` was learned from PID1F as tank-heater toggle. Muted PID5E injection proved that the PMM accepts this command and briefly turns the tank heater on. The touchscreen then reasserts its own internally configured state and turns it back off.

Conclusion: PID5E-only tank-heater control is blocked by the touchscreen ownership model, likely because the touchscreen owns the tank-heater voltage policy. The test button has been removed from Build 012.0.
