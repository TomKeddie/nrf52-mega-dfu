#include "bootloader_util.h"


#if !defined(MEGADFU_START)
#error MEGADFU_START must be defined!
#endif


int main(void) {
  // S132 version 2.0 occupies 0000_0000-0001_b6b0
  // S132 version 7.2 occupies 0000_0000-0002_596c
  // SDK11 will place this code at 0001_c000 which will be overwritten when the softdevice is updated
  // Here we use the booloader to start the updater application at
  // MEGADFU_START (0x0002_6000) which is above S132 version 7.2

  // Just forward on to MegaDFU, which is located at an offset that won't be obliterated by the incoming SoftDevice update
  bootloader_util_app_start(MEGADFU_START);

  return 0;
}
