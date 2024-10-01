/* Linker script to configure memory regions. */
/* Based on SDK11 ble_app_hrs_gcc_nrf52.ld */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x77000, LENGTH = 0x1000 /* must end before the bootloader */
  RAM (rwx) :  ORIGIN = 0x20002080, LENGTH = 0xdf80
}

SECTIONS
{
  .fs_data :
  {
    PROVIDE(__start_fs_data = .);
    KEEP(*(.fs_data))
    PROVIDE(__stop_fs_data = .);
  } > RAM
} INSERT AFTER .data;

/* TODO: Place payload sections */

INCLUDE "nrf5x_common.ld"