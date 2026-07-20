# PID32 - Output Bitmap

PID32 reports coach output state bits. It is the primary feedback source for non-HVAC outputs and is used by the PID1F command learner to infer touchscreen command intent.

## Confirmed bit mapping

| LIN data byte | Bit | Decode |
|---:|---:|---|
| Byte 0 | `0x01` | Awning Light |
| Byte 0 | `0x10` | Water Heater |
| Byte 0 | `0x40` | Tank Heater |
| Byte 0 | `0x80` | Water Pump |
| Byte 1 | `0x02` | Awning Retract |
| Byte 1 | `0x04` | Awning Extend |
| Byte 1 | `0x80` | Sofa Retract |
| Byte 2 | `0x01` | Sofa Extend |
| Byte 2 | `0x02` | Wardrobe Slide Retract |
| Byte 2 | `0x04` | Wardrobe Slide Extend |
| Byte 2 | `0x08` | Bedroom Slide Retract |
| Byte 2 | `0x10` | Bedroom Slide Extend |
| Byte 6 | `0x40` | Generator Running |

## Notes

- The output bitmap is useful for validating PID1F commands.
- A PID1F command followed by a PID32 diff can be automatically labeled by the learner.
- All slide and awning direction bits were revalidated with isolated PID5E/PID32
  flight-recorder captures on 2026-07-20.
