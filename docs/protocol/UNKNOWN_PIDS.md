# Unknown / Auxiliary PIDs

## Open investigation: complete generator runtime

PIDBA currently exposes the generator state in the page high nibble and only
the runtime tenths digit in the page low nibble. Capture and correlate all LIN
PIDs across known displayed-runtime increments to locate the remaining runtime
digits or counter. Include the `x.9` to next-hour rollover and test for
multi-frame, packed-BCD, binary-counter, and rolling-page encodings.

## PID76

Repeating mostly-zero frame. Needs mapping.

## PID9C

Active. Current interpretation: scheduler / HVAC auxiliary candidate. Needs further validation.

## PIDDD

Active. Current interpretation: scheduler / housekeeping candidate. Needs further validation.
