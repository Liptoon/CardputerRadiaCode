# CardputerRadiaCode

BLE RadiaCode-102 dosimeter/spectrometer helper for M5CardputerADV.

Built with PlatformIO (Arduino + ESP32-S3).

## Features

- BLE scan and connect to RadiaCode-102
- Live dose rate, cumulative dose, CPS display
- Gamma spectrum histogram
- Geiger clicker (CPS-driven Poisson)
- SD card CSV logging
- Status bar with Cardputer & RadiaCode battery, BLE status
- Brightness control, geiger toggle, reset dose
- Keyboard navigation (Enter, Tab, Space, Backspace)

## Building

```bash
pio run
```

No upload — only compile tested.

## Hardware

- M5CardputerADV (ESP32-S3)
- PlatformIO board: `m5stack-cores3`

## Credits

- **cardputer-framework** — keyboard/screen/splash utility library (copied as-is into `src/`)
- RadiaCode protocol reference by the RadiaCode community

## License

MIT
