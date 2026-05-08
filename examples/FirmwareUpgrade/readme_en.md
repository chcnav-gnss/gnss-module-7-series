# FirmwareUpgrade

`FirmwareUpgrade` provides a Linux-based reference demo for upgrading firmware on CHCNAV 7 Series GNSS modules. It is intended for OEM customers, system integrators, and developers who need host-side example code for serial upgrade workflows.

## Contents
- Upgrade entry and workflow control: `UpgradeEntry.c`, `UpgradeM7xx.c`
- Upgrade protocol handling: `UpgradeProtocol.c`, `UpgradeProtocol.h`
- Serial communication helpers: `UartCommon.c`, `UartCommon.h`
- Integrity helpers: `CRCGenerate.c`, `md5.c`
- Build files: `Makefile`, `build.sh`

## Build Prerequisites
- Linux build environment
- GCC toolchain
- `make`
- Access to the target serial device node
- A valid `.pkg` firmware package for the target module

## Build
### Native Linux build
```bash
cd examples/FirmwareUpgrade
make
```

The default output binary is:
```bash
update_m7xx
```

### Cross-compilation
If you need cross-compilation, update the `CC` setting in `Makefile` to match your toolchain.

`build.sh` is also provided as a reference script for embedded Linux build environments that already have a matching cross-compilation setup.

## Command Usage
This tool is intended for Linux platforms.

Command format:
```bash
./update_m7xx -o -d <device_path> -b <baudrate> -p <package_path> -t <transfer_size> -r -c -f -u
```

Examples:
```bash
./update_m7xx -o -d /dev/ttySP1 -b 921600 -p Enc_M722_V2.1.31.1.pkg -t 2048 -r -c -f
./update_m7xx -o -d /dev/ttySP1 -b 921600 -p Enc_M722_V2.1.31.1.pkg -t 2048 -r -c -f -u
```

## Parameters
- `-o`: output debug information; omit this option for quiet mode
- `-d`: target device node
- `-b`: communication baud rate; supported values are `9600`, `19200`, `115200`, `460800`, `921600`, `1500000`, `3000000`
- `-p`: firmware package path
- `-t`: valid payload length per transfer frame; supported values are `1024`, `2048`, `4096`, `8192`, `16384`, `32768`
- `-r`: reboot the module immediately after upgrade
- `-c`: enable fast upgrade mode
- `-f`: force upgrade; ignores version comparison and allows downgrade
- `-u`: enable remote device firmware upgrade for relay scenarios such as LoRa transparent transmission

## Recommended Settings
- Use `-t 2048` for a balanced transfer size
- Enable `-r` to avoid manual reset after upgrade
- Enable `-c` to reduce upgrade time
- Enable `-f` when downgrade compatibility is required
- Use `-u` only for remote relay upgrade scenarios; direct serial upgrade normally does not require it

## Notes
- Confirm the firmware package matches the target module model before upgrading.
- Verify serial port permissions and port occupancy before starting the upgrade.
- Validate the full upgrade flow in a controlled environment before field deployment.
