# ESPHome Precision Plex LIN v0.5.0

This is the first GitHub-hosted external-component release of the Precision
Plex LIN Analyzer. It preserves the field-tested behavior of legacy Build
013.1 while providing a clean, tag-pinned ESPHome installation path.

## Included

- Versioned `precision_plex_lin` ESPHome external component.
- Complete production configuration retaining the existing ESPHome node and
  Home Assistant entity contract.
- Corrected bedroom-slide extend/retract and sofa-extend PID32 mappings.
- Verified patio-awning, wardrobe-slide, and sofa-retract direction mappings.
- LIN-preferred telemetry support for Precision Plex integration v5.4.2.
- PID1F/PID5E command learners and 10-second flight recorder.
- Proven muted PID5E awning-light diagnostic test.
- Hardware, installation, architecture, repository-layout, and protocol docs.

## Command status

Normal operation remains telemetry-focused. Observed motor-command payloads
are documented, but motor controls are not exposed until release and explicit
stop behavior have been captured and validated.

## Upgrade from Build 013.1

Use `esphome/examples/precision_plex_lin_analyzer.yaml` as the replacement
configuration. Preserve the existing node name and pin assignments, then flash
the node normally through ESPHome. The Home Assistant entity names consumed by
Precision Plex v5.4.2 remain unchanged.
