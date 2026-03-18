# Changelog

All notable changes to this project will be documented in this file.

## [1.0] - 2026-03-17
### Added
- Initial project creation with PlatformIO structure.
- Firmware for ESP32-C3 to control Modbus RTU motor drivers via MAX485.
- Control interface via AsyncWebServer accessible on Access Point (`CEA-MotorControl`).
- Dynamic speed variation with `gap` and `delta` logic.
- Persistence of user configurations using `Preferences`.
- Hardware digital inputs (IN1, IN2) for manual physical control.
- Auto-off logic for the Access Point after 5 minutes of inactivity to save power.
- English web UI translation and optimized button layout.
