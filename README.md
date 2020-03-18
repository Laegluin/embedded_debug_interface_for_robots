# Embedded Debug Interface for Robots

This repository contains the source code for the Embedded Debug Interface for Robots bachelor thesis.
Refer to it for if you want to know exactly what this repository is all about. You can find the LaTeX
sources for it in the `thesis` directory.

TL;DR: This is firmware and a bootloader for the STM32F7508-DK development board. It allows you to
see what devices are connected to a [ROBOTIS Dynamixel 2.0][dynamixel-protocol] based bus and what
their current status is. The bus must be connected to `UART6` (the `D0` pin at the back of the board).
Make sure to use an RS-485 transceiver if you're using the RS-485 variant.

## Building

You will need the following packages (Ubuntu);

- gcc
- binutils
- gcc-arm-none-eabi
- binutils-arm-none-eabi
- STM32CubeF7 1.15.0 (download from [here][stm32cubef7])
- gdb-multiarch (debugging and flashing)
- clang-format (formatting only)

Download the STM32CubeF7 zip and unzip it to `vendor`. Rename the directory to `stm32_cube_f7_1.15.0`.

You can then build the firmware with `make`, clean the output directory with `make clean`, run tests
with `make test` and format the code with `make format`. The resulting firmware is `target/firmware.elf`
(for debugging) and `target/firmware.bin` (binary image for flashing).

Building the bootloader works the same way, just cd into the `bootloader` directory first. The bootloader
binaries are `target/bootloader/bootloader.elf` and `target/bootloader/bootloader.bin`.

## Flashing

You need to flash the bootloader first. The easiest way to do this is to use `gdb-multiarch`. To use
GDB, you need to have a GDB server running. You can get one by downloading and installing
[STM32CubeIDE][stm32cubeide] and [STM32CubeProgrammer][stm32cubeprogrammer]. You can find the binary
at `plugins/com.st.stm32cube.ide.mcu.externaltools.stlink-gdb-server.win32_1.1.0.201910081157/tools/bin`
(you may have to change exact path depending on the version you're using). Start it with
`./ST-LINK_gdbserver -v --swd -e -cp <path to STM32CubeProgrammer>/bin`. In GDB you can connect to
the server with `target extended-remote :61234`. A simple `load` will flash the bootloader (assuming
you have selected `bootloader.elf` for debugging).

The bootloader is controlled through the `USB_FS` port (next to the ST-Link port). Connect a USB cable
and it will show up as USB serial device. The boards LED will blink when the bootloader is running.
You can use the `etc/50-usb-serial.rules` udev rule to automatically configure permissions for the
device. If you do not use the udev rules or flashing does not work, make sure that the ModemManager
service is disabled or not installed. It likes to mess with all serial devices it can get its hands
on.

You can flash the firmware with `make flash` and start it with `make start`. `make run` flashes and
then immediately starts the firmware. Alternatively, you can also start it by clicking the button
next to the reset button. When the firmware has started the board's LED will be turned on.

If you do not use the udev rules file, you will have to specify the path to the bootloader's USB
serial device. For example:

```sh
export SERIAL=/dev/ttyS4
# or
SERIAL=/dev/ttyS4 make run
```

## Adding Support for a New Device

To add a new device, create a new file and header in `src/device`. Define a class that derives from
`ControlTable` and implement the missing methods. You can look at the other devices for examples.
You should also add a static `MODEL_NUMBER` member that is set to the model number of the device.

Once you have implemented the `ControlTable` interface, you'll have to add your class to the
`ControlTableMap::register_control_table` method in `src/control_table.cpp`. Add a case for the new
device and you're done!

## License

Licensed under either of

- Apache License, Version 2.0
   ([LICENSE-APACHE](LICENSE-APACHE) or <http://www.apache.org/licenses/LICENSE-2.0>)
- MIT license
   ([LICENSE-MIT](LICENSE-MIT) or <http://opensource.org/licenses/MIT>)

at your option.


[dynamixel-protocol]: http://emanual.robotis.com/docs/en/dxl/protocol2/
[stm32cubef7]: https://www.st.com/en/embedded-software/stm32cubef7.html
[stm32cubeide]: https://www.st.com/en/development-tools/stm32cubeide.html
[stm32cubeprogrammer]: https://www.st.com/en/development-tools/stm32cubeprog.html
