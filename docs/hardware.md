# Hardware

## Reference connections

The field-tested topology is:

```text
ESP32 <-> LIN transceiver <-> Precision Plex LIN bus
```

| Function | ESP32 pin |
|---|---:|
| LIN UART receive | GPIO16 |
| LIN UART transmit | GPIO17 |
| Wireless TP FAULT/TXE claim | GPIO25 through 1 kOhm |

The ESP32, transceiver, and coach must share ground. The LIN transceiver must be
rated and powered for the coach-side LIN voltage; never connect the coach LIN
wire directly to an ESP32 GPIO.

## Wireless TP claim line

GPIO25 is connected through a 1 kOhm resistor to the Wireless TP MCP2004E
FAULT/TXE test pad. Firmware uses open-drain-style behavior:

- Drive low briefly to prevent the factory Wireless TP from answering its
  PID5E response slot.
- Return the GPIO to input/high-impedance immediately after the injected
  response.
- Never drive this line high.

This path has been proven with the safe awning-light toggle. It is retained for
command research but is not yet used for motor controls.
