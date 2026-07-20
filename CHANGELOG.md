# Changelog

## v0.5.0 - GitHub External Component Baseline

- Converts the field-tested LIN Analyzer Build 013.1 into a versioned ESPHome
  external component.
- Preserves automatic LIN parsing, telemetry entities, freshness signals,
  flight recording, and the safe PID5E awning-light command test.
- Includes the corrected PID32 direction mappings for the bedroom slide and
  sofa extend.
- Adds a production example pinned to the `v0.5.0` GitHub release tag.
- Records the observed PID5E motor-command sequence while leaving motor control
  disabled pending stop/release captures.

The full legacy build history is retained in
[`docs/protocol/CHANGELOG.md`](docs/protocol/CHANGELOG.md).
