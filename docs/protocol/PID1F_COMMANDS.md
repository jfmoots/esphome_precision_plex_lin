# PID1F Touchscreen Commands

PID1F is the Precision Plex touchscreen command channel. Commands observed so far are momentary, high-level semantic commands. The PMM performs interlocks, sequencing, and output actuation after receiving the command.

## Confirmed command map

| Payload | Command | Evidence |
|---|---|---|
| `3F 00` | Idle / ready | Repeating idle frame |
| `00 00` | Awning light toggle | PID32 Byte 0 `0x01` toggles |
| `04 00` | Water heater toggle | PID32 Byte 0 `0x10` toggles |
| `06 00` | Tank heater toggle | PID32 Byte 0 `0x40` toggles; touchscreen ownership limits external control |
| `07 00` | Water pump toggle | PID32 Byte 0 `0x80` toggles |
| `49 00` | Patio awning retract | PID32 awning retract motion bit |
| `4A 00` | Patio awning extend | PID32 awning extend motion bit |
| `4F 00` | Sofa slide retract | PID32 sofa retract motion bit |
| `50 00` | Sofa slide extend | PID32 sofa extend motion bit |
| `51 00` | Wardrobe slide retract | PID32 wardrobe retract motion bit |
| `52 00` | Wardrobe slide extend | PID32 wardrobe extend motion bit |
| `53 00` | Bedroom slide retract | PID32 bedroom retract motion bit |
| `54 00` | Bedroom slide extend | PID32 bedroom extend motion bit |

The lower initial forms `09/0A`, `0F/10`, `11/12`, and `13/14` and their
corresponding `+0x40` active forms are normalized to the same logical movement.
Repeated holds are ignored. An idle `3F 00` publishes a release only when
PID1F previously announced an active motion.

## Notes

All confirmed commands currently use the form `XX 00`, suggesting byte 0 is the command opcode and byte 1 is unused or reserved for these actions.

Motion commands are grouped in the `0x49` through `0x54` range, with gaps that may represent unsupported hardware, stop/cancel functions, service functions, or other coach-specific commands.

## Retired timing-race work

Earlier builds attempted PID1F timing sweeps to beat the touchscreen response slot. That approach is retired. The current architecture uses the Wireless TP PID5E slot and the MCP2004E FAULT/TXE claim line instead of racing an installed controller.

## Generator commands pending

Generator commands have been captured on the BLE control path, but their LIN
intent opcodes are not yet included in the confirmed PID1F/PID5E observer map:

- Generator Start
- Generator Stop
- Generator AutoStart Enable
- Generator AutoStart Disable

PIDBA generator observer remains useful for correlating generator state/runtime changes with learned commands.
