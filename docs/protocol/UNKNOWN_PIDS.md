# Unknown / Auxiliary PIDs

## Validation watch: generator runtime rollover

PIDBA now decodes the complete generator runtime from data bytes 1-3 and the
page low nibble. The `125.4` LIN value exactly matched Bluetooth. Capture the
future `125.9 -> 126.0` rollover to validate the whole-hour carry behavior.

## PID76

Repeating mostly-zero frame. Needs mapping.

## PID9C

Active. Current interpretation: scheduler / HVAC auxiliary candidate. Needs further validation.

## PIDDD

Active. Current interpretation: scheduler / housekeeping candidate. Needs further validation.
