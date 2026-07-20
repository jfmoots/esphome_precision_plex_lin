# PIDBA - Coach Telemetry

PIDBA reports coach telemetry including house battery, tanks, propane, and generator state/runtime information.

## Confirmed byte / nibble mapping

| LIN field | Decode |
|---|---|
| Page high nibble | Generator state |
| Page low nibble | Generator runtime tenths digit |
| Byte 7 | House battery voltage |
| Byte 8 high nibble | Propane |
| Byte 8 low nibble | Fresh tank |
| Byte 9 high nibble | Black tank |
| Byte 9 low nibble | Grey tank |

## Scaling

```text
Battery voltage = raw Byte 7 / 16.0
```

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
