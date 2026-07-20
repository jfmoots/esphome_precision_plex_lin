# PID37 - HVAC Status

PID37 reports the state of the Precision Plex HVAC zones. It is telemetry only. HVAC control is still owned by the touchscreen; the Wireless TP / BLE path does not expose HVAC control.

## Fields

| Field | Status |
|---|---|
| Zone | Confirmed |
| B4 Mode | Mostly confirmed |
| B5 Request / Phase | Partially decoded; `0x50` needs more validation |
| B6 Operating State | Partially decoded |
| Room Temp | Confirmed |
| Setpoint | Confirmed |
| Compressor Restart Delay Timer | Confirmed |
| Byte 10 | Constant `0x06` observed |

## B4 mode values

| Zone | Hex | Decode |
|---|---:|---|
| 1 | `00` | Off |
| 1 | `10` | Auto Heat candidate |
| 1 | `11` | Heat Pump |
| 1 | `12` | Furnace |
| 1 | `20` | Cool |
| 2 | `01` | Off |
| 2 | `11` | Heat Pump |
| 2 | `21` | Cool |

## B5 request / phase values

| Hex | Decode |
|---:|---|
| `00` | idle / manual / no active request |
| `01` | heat pump request/context |
| `10` | cooling request/context |
| `04` | furnace heat active |
| `50` | needs validation: observed with satisfied cooling + restart delay; may be generic satisfied/waiting context |
| `54` | furnace call / ignition sequence |

### B5 research note

`0x50` was originally associated with furnace/forced-fan context, but it has also been captured while Zone 1 was in `cool_satisfied` with the compressor restart delay active. Watch for `0x50` during satisfied heating cycles. It may be a generic controller satisfied/waiting context rather than a furnace-specific phase.

## B6 operating state values

| Hex | Decode |
|---:|---|
| `00` | off idle |
| `11` | fan low running |
| `37` | fan high running |
| `40` | cool selected / satisfied |
| `41` | cooling active, low fan |
| `47` | cooling active, high fan |
| `50` | heat / furnace selected or waiting |
| `51` | heat pump active, low fan |

## Compressor restart delay

PID37 byte 9 is better described as a compressor anti-short-cycle restart delay timer. Values greater than zero mean the compressor is temporarily prevented from restarting after a stop/satisfied cycle.

## Byte 10

`0x06` has been observed across all captured operating states so far. Current interpretation: static configuration or reserved field.

## Furnace note

PID37 does not appear to report furnace blower state. Furnace phase is inferred from B5 only.
