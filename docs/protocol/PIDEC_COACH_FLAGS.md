# PIDEC - Coach Flags

PIDEC reports coach-level flags.

## Confirmed decode

| LIN field | Bit | Decode |
|---|---:|---|
| Byte 2 / B5 | `0x04` | AC / Converter Present |
| Byte 2 / B5 | `0x01` | Ignition On |

## Notes

- AC / Converter Present is useful for power-source inference.
- Ignition On should gate any alternator/engine-running inference.
