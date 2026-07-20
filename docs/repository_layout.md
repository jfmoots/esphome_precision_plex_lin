# Repository Layout

```text
README.md
CHANGELOG.md

docs/
  architecture.md
  hardware.md
  installation.md
  repository_layout.md
  protocol/                    Reverse-engineered PID references

dashboards/
  lin_workbench.yaml           Home Assistant research dashboard

esphome/
  components/
    precision_plex_lin/
      __init__.py              ESPHome schema and code generation
      precision_plex_lin.h     Component lifecycle and UART adapter
      precision_plex_lin.cpp
      version.h
      analyzer.h               Stream parser, learner, and flight recorder
      coach_state.h            Decoded state and freshness tracking
      pid32.h                  Output and movement bitmap
      pid37.h                  HVAC telemetry
      ba.h                     Coach/generator telemetry
      ec.h                     Coach power flags
      pid1f.h                  Touchscreen command learner
      scheduler.h              Auxiliary scheduler decoding
      protocol.h               Frame and checksum helpers

  examples/
    precision_plex_lin_analyzer.yaml
    secrets.example.yaml
```
