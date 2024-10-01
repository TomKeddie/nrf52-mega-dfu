# Proxxi CT Firmware Bootstrap
Firmware required to update a factory-stock ID130C with iDO's firmware to Proxxi's CT firmware through a series of DFU images and a bit of voodoo.

## sdk11-bootloader/bootloader
Stock DFU bootloader from SDK11 with a few modifications:
* Single-bank operation to allow for larger DFU packages than are possible with dual-bank.
* Bug fix for an issue that prevents updating the SoftDevice (not strictly necessary for our use case).
* Start address is configured with an environment variable.

## sdk11-bootloader/dfu-shotgun
Utility DFU application, primarily for development purposes, that includes a payload of many different bootloaders with different start addresses. Its primary function is to extract and install the appropriate bootloader for the device that it is running on as specified by the bootloader start address in the UICR. This is so we can always install a bootloader on an unknown device.
Presents itself as a BLE UART service with various commands useful for understanding the host device:
* b: Update bootloader
* d: Reset for DFU mode
* e: Erase bootloader settings page
* f: Dump FICR
* r: Read bootloader address and MBR settings page address
* s: Read SoftDevice information (size, fwid)
* u: Dump UICR
* w: Dump the watchdog state
* 1: Dump the bootloader
* 2: Dump the MBR settings page
* 3: Dump the bootloader settings page

## megadfu
Packages the Proxxi CT firmware, bootloader, and appropriate SoftDevice (the payloads), and installs and configures them when run.
Requres the following files to be copied to megadfu/ from Proxxi CT's firmware build output directory:
* bootloader/bootloader.hex
* bootloader_setting.hex
* firmware.hex
* dfu-package-app-all.DONOTUSETHISFILE.zip/sd_bl.bin

## megadfu-bootstrap
Firmware used to jump from the SDK11/SDv200 application start address to megadfu that is located at the SDK15.3/SDv611 application start address, as the SDK11 application start address will be overwritten when the new SoftDevice is installed.

## megadfu-finalize
Firmware used to perform the actual install of the packaged Proxxi CT firmware, bootloader, and SoftDevice. Will be installed immediately before the bootloader by megadfu, then jumped to. Payload addresses and sizes are passed via UICR->Customer[22..31].

# Memory Map

    softdevice        0x00000000
                      0x0001b6b0
    megadfu-bootstrap 0x0001c000
                      0x0001c914
    megadfu           0x00026000
                      0x00075000
    megadfu-finalise  0x00075000
                      0x00075fff
    bootloader616     0x00076000
                      0x0007ffff
