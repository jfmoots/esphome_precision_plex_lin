# PIDBA - Coach Telemetry

PIDBA reports coach telemetry including house battery, tanks, propane, and generator state/runtime information.

## Confirmed byte / nibble mapping

| LIN field | Decode |
|---|---|
| Page high nibble | Generator state |
| Page low nibble | Generator runtime tenths digit |
| Data bytes 1-3 | Generator whole hours, little-endian packed BCD |
| Byte 7 | House battery voltage |
| Byte 8 high nibble | Propane |
| Byte 8 low nibble | Fresh tank |
| Byte 9 high nibble | Black tank |
| Byte 9 low nibble | Grey tank |

## Scaling

```text
Battery voltage = raw Byte 7 / 16.0
```

## Generator runtime

The complete counter is formed from three little-endian packed-BCD bytes plus
the page low nibble. Every BCD nibble is validated before publication.

```text
PIDBA data: 14 25 01 00 D0 30 00 FF
state:      0x1 = running
whole:      00 01 25 BCD = 125 hours
tenths:     0x4 = 0.4 hours
runtime:    125.4 hours
```

This value matches the independently decoded Bluetooth runtime. The future
`125.9 -> 126.0` rollover remains the final whole-hour carry validation.

## Tank code

```text
0x0 = 0%
0x2 = 33%
0x4 = 67%
0x6 = 100%
```

## Propane code

```text
0x0 = 0%
0x1 = 25%
0x3 = 50%
0x5 = 75%
0x6 = 100%
```
