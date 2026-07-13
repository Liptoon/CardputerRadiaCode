# CardputerRadiaCode

**v1.0.0** — BLE RadiaCode-102 dosimeter/spectrometer helper for M5CardputerADV.

Built with PlatformIO (Arduino + ESP32-S3).

## Features

- BLE scan and connect to RadiaCode-102 (auto-reconnect)
- Live dose rate (µSv/h), cumulative dose (µSv), CPS display
- Gamma spectrum histogram (1024 channels, V0/V1 decoding)
- Geiger clicker (CPS-driven Poisson, realistic GM-tube sound)
- SD card CSV logging
- Status bar with Cardputer & RadiaCode battery, device time (HH:MM), BLE status
- Alarm detection (DR/DOSE levels from device flags) with two-tone beep
- Brightness control (4 levels), geiger toggle, alarm mute, dose reset
- Thin border UI, keyboard navigation (Enter/Tab/Space/Backspace)
- Persistent settings (NVS + SD fallback)

## Building

```bash
pio run
```

Flashing via M5 Launcher: copy `firmware.bin` to SD card folder and flash from device.

## Hardware

- M5CardputerADV (ESP32-S3)
- PlatformIO board: `m5stack-cores3`

## Limitations (future versions)

- Spectrum energy calibration (keV axis) — planned
- Spectrum zoom/scroll — planned
- Wall-clock time via NTP — requires WiFi connection

## Release binary

Pre-built binaries are attached to GitHub releases as
`RadiacodeCardputer_v<version>.bin`.

## Credits

- **cardputer-framework** — keyboard/screen/splash utility library
- RadiaCode protocol reference by the RadiaCode community

## License

MIT
