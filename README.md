gnss-module-7-series
=========

[![Release][release-image]][releases]
[![License][license-image]][license]

[release-image]: https://img.shields.io/github/release/chcnav-gnss/gnss-module-7-series
[releases]: https://github.com/chcnav-gnss/gnss-module-7-series/releases

[license-image]: https://img.shields.io/github/license/chcnav-gnss/gnss-module-7-series.svg
[license]: https://github.com/chcnav-gnss/gnss-module-7-series/blob/main/LICENSE

`gnss-module-7-series` is an open-source repository from CHCNAV for customer integration of 7 Series GNSS modules. It contains example code and supporting materials for common host-side integration workflows such as firmware upgrade, communication, and feature enablement.

The repository root is intentionally kept concise. Detailed build guides, usage notes, and protocol-specific examples are maintained inside each feature directory so the project can grow without making the top-level README difficult to navigate.

## Repository Overview
- `examples/`: sample projects for specific integration scenarios
- `examples/FirmwareUpgrade/`: Linux-based firmware upgrade demo for M7xx modules
- `LICENSE`: Apache 2.0 license for public use and redistribution

## Documentation Entry Points
- Firmware upgrade guide (English): [`examples/FirmwareUpgrade/readme_en.md`](examples/FirmwareUpgrade/readme_en.md)
- Firmware upgrade guide (Chinese): [`examples/FirmwareUpgrade/readme_zh.md`](examples/FirmwareUpgrade/readme_zh.md)

## Project Structure
```text
gnss-module-7-series
|-- examples
|   `-- FirmwareUpgrade
|       |-- main.c
|       |-- UpgradeEntry.c
|       |-- UpgradeM7xx.c
|       |-- UpgradeProtocol.c
|       |-- UartCommon.c
|       |-- CRCGenerate.c
|       |-- md5.c
|       |-- Makefile
|       |-- build.sh
|       |-- readme_en.md
|       `-- readme_zh.md
|-- LICENSE
`-- README.md
```

## Notes
- Each example directory should own its detailed usage guide and build instructions.
- The root README should remain a project-level index as new demos and tools are added.
- Before using any example, verify module model compatibility, serial settings, and package or data format requirements.

## License
This project is released under the Apache 2.0 License. See [LICENSE](LICENSE) for details.

## Feedback & Contribution
Feedback, bug reports, and contributions are welcome.
Please submit Issues or Pull Requests to help improve `gnss-module-7-series`.
