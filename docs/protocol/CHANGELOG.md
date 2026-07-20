# Changelog

## Build 013.1 - Motion Telemetry Decode Corrections

- Corrects bedroom slide extend to PID32 Byte 2 `0x10`.
- Corrects bedroom slide retract to PID32 Byte 2 `0x08`.
- Corrects sofa extend to PID32 Byte 2 `0x01`.
- Revalidates sofa retract, both wardrobe directions, and both patio-awning
  directions against isolated PID5E/PID32 flight-recorder captures.
- Records the observed two-phase PID5E movement command payloads for future
  wired-control work; command injection behavior remains unchanged.

## Build 013.0 - Home Assistant Telemetry Transport

- Adds 10-second freshness-qualified `LIN Telemetry Active` and
  `LIN Output State Active` entities.
- Adds separate extend/retract entities for the patio awning, sofa slide,
  bedroom slide, and wardrobe slide.
- Provides the entity contract used by Precision Plex integration v5.4.0 for
  automatic LIN-preferred telemetry with Bluetooth fallback.
- Leaves command injection behavior unchanged for continued investigation.

## Build 012.0 - Wired Interface Cleanup

- Removes obsolete PID1F slot timing sweep controls from the dashboard.
- Removes the experimental PID5E tank-heater test button and firmware service.
- Keeps the safe PID5E awning-light test as the primary wired command sanity test.
- Updates documentation for the Wireless TP MCP2004E FAULT/TXE claim line.
- Documents the tank-heater finding as a closed PID5E-only investigation due to touchscreen ownership/voltage-policy behavior.
- Keeps the 10-second flight recorder, PID1F/PID5E learners, PID32/PID37/PIDBA decoders, and TX diagnostics.

## Build 011.1 - PID5E Tank Heater Test

- Added a temporary tank-heater test using PID5E payload `06 00`.
- Result: PMM accepted the command and turned the tank heater on briefly.
- Finding: touchscreen reasserted its configured state and turned the tank heater back off.
- The test was removed in Build 012.0.

## Build 011.0 - Wireless TP Claim Command Path

- Added GPIO25 control of the Wireless TP MCP2004E FAULT/TXE line for PID5E command injection.
- PID5E awning-light test now claims the Wireless TP slot, waits for the next PID5E poll, sends `00 00 A1`, then releases the claim after TX completes.
- Firmware uses open-drain style control: GPIO25 drives low to claim and floats/input to release.
- Expected wiring: ESP GPIO25 -> 1k resistor -> Wireless TP MCP2004E FAULT/TXE test pad, with common GND.

## Build 010.8 - Analyzer State Cleanup

- Treats PID5E `B0 02` and `B1 01` as Wireless TP housekeeping.
- Suppresses PID5E housekeeping correlation timers and repeated no-change log spam.
- Keeps PID5E command injection test and 10-second flight recorder.

## Build 010.6 - PID5E Fast TX

- Added raw byte-stream PID5E fast-response path.
- Sends queued PID5E payload as soon as `00 55 5E` is observed.
- Kept the parser-level PID5E transmit as fallback.

## Build 010.5 - 10s Flight Recorder

- Extended manual all-PID flight recorder from 2 seconds to 10 seconds.
- Retains up to 500 relevant frames for human-in-the-loop captures.

## Build 010.3 - All-PID Flight Recorder

- Added manual all-PID flight recorder.
- Captures PID1F, PID5E, PID32, PID37, PIDBA, PIDEC, PID76, PID9C, and PIDDD context frames.

## Build 010.2 - PID5E Wireless TP / BLE Bridge Analyzer

- Added PID5E non-idle payload learner.
- Added PID5E correlation against PID32 outputs, PID37 HVAC state, and PIDBA generator telemetry.
- Added diagnostic entities for PID5E last non-idle command, last learned command, and command history.

## Build 010.0 - PID1F Timing Sweep

- Added PID1F slot timing experiments to test whether the ESP could beat the touchscreen.
- This approach was later retired after discovering the Wireless TP FAULT/TXE claim mechanism.
