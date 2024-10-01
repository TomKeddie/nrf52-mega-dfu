#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "app_util_platform.h"
#include "nrf_wdt.h"
#include "bootloader_util.h"

#include "prx_nvmc.h"

#include <nrf52.h>
#include <nrf52_bitfields.h>

#include "itm_messages.h"


#define DEBUG 1

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)


// these externs are all resolved when this application is linked with megadfu-finalise
extern const uint32_t _binary__build_obj_payload_descriptor_bin_start;
extern const uint32_t _binary__build_obj_payload_descriptor_bin_end;
extern const uint32_t _binary__build_obj_payload_finalize_bin_start;
extern const uint32_t _binary__build_obj_payload_finalize_bin_end;
extern const uint32_t _binary__build_obj_payload_softdevice_lz4_start;
extern const uint32_t _binary__build_obj_payload_softdevice_lz4_end;
extern const uint32_t _binary__build_obj_payload_bootloader_lz4_start;
extern const uint32_t _binary__build_obj_payload_bootloader_lz4_end;
extern const uint32_t _binary__build_obj_payload_settings_lz4_start;
extern const uint32_t _binary__build_obj_payload_settings_lz4_end;

// these values are all resolved when this application is linked with megadfu-finalise
// In the Makefile
//  - struct_builder.py takes the supplied values and generates a binary file payload_descriptor.bin
//  - these are converted to a .o with objcopy
//  - the pointers above are resolved during linking
typedef struct {
	unsigned char* finalize_start;
	unsigned char* sd_start;
	unsigned char* bl_start;
	unsigned char* settings_start;
	unsigned char* app_start;
} PayloadDescriptor_t;

void WDT_IRQHandler() {
    if (nrf_wdt_int_enable_check(NRF_WDT_INT_TIMEOUT_MASK) == true) {
        nrf_wdt_event_clear(NRF_WDT_EVENT_TIMEOUT);
    }
}

void start_etm(void) {
  const unsigned ETMBASE = 0xE0041000;
  // Allow access to device
  *(unsigned*)(ETMBASE+0xfb0) = 0xc5acce55;

  // Enter configuration mode (write twice to be sure we reached it)
  *(unsigned*)(ETMBASE) = (1<<10);
  *(unsigned*)(ETMBASE) = (1<<10);

  // Set busID 2
  *(unsigned*)(ETMBASE+0x200) = 2;

  // Set trigger event
  *(unsigned*)(ETMBASE+8) = 0x406f;

  // Set to always enable in ETM Trace Enable Event
  *(unsigned*)(ETMBASE+0x20) = 0x6f;

  // Trace and stall always enabled
  *(unsigned*)(ETMBASE+0x24) = 0x020000001;

  // Stall when < 8 byes free in fifo
  *(unsigned*)(ETMBASE+0x2c) = 8;

  // Enable trace
  *(unsigned*)(ETMBASE) = 0x0800 | (1 << 7) | (1 << 8);

  // Essential that this bit is only cleared after everything else is done
  *(unsigned*)(ETMBASE) &= ~(1<<10);
}

void trace_init(void) {
  // enableNRF52TRACE 4 3 3

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

  NRF_CLOCK->TRACECONFIG |= (CLOCK_TRACECONFIG_TRACEMUX_Parallel << CLOCK_TRACECONFIG_TRACEMUX_Pos) | (CLOCK_TRACECONFIG_TRACEPORTSPEED_32MHz << CLOCK_TRACECONFIG_TRACEPORTSPEED_Pos);

  NRF_P0->PIN_CNF[18] = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
  NRF_P0->PIN_CNF[20] = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
  NRF_P0->PIN_CNF[14] = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
  NRF_P0->PIN_CNF[15] = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
  NRF_P0->PIN_CNF[16] = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);

  // unload ITM
  ITM->LAR = 0xc5acce55;

  // Set port size (TPIU_CSPSR)
  TPI->CSPSR = 1 << 3;
  // Set pin protocol to Sync Trace Port (TPIU_SPPR)
  TPI->SPPR = 0;

  TPI->FFCR = 0x102;

  ITM->TCR &= ~(0x7f << 16);
  ITM->TCR |= (1 << 16);

  // https://developer.arm.com/documentation/ddi0403/d/Debug-Architecture/ARMv7-M-Debug/The-Data-Watchpoint-and-Trace-unit/Control-register--DWT-CTRL?lang=en
  // dwtSamplePC 1
  DWT->CTRL |= (1 << 12);

  // dwtTraceException 1
  DWT->CTRL |= (1 << 16);
  
  // dwtSyncTap 1 (was 3)
  DWT->CTRL &= ~(3 << 10);
  DWT->CTRL |= (1 << 10);

  // dwtPostTap 0
  DWT->CTRL &= ~(1 << 9);
  
  // dwtPostInit 0 (was 1)
  CoreDebug->DCRDR |= 0x1000000;
  DWT->CTRL |= ~(0x0f << 5);
  //  DWT->CTRL &= ~(1 << 5);
  
  // dwtPostReset 0 (was 10)
  DWT->CTRL &= ~(0x0f << 1);
  //  DWT->CTRL |= (0x00 << 1);

  // dwtCycEna 1
  DWT->CTRL |= (1 << 0);

  // ITMTXEna 1
  ITM->TCR |= (1 << 3);

  // ITMEna 1
  ITM->TCR |= (1 << 0);



  start_etm();
}

static inline void bootloader_util_reset_local(uint32_t start_addr)
{
    __asm volatile(
        "ldr   r0, [%0]\t\n"            // Get App initial MSP for bootloader.
        "msr   msp, r0\t\n"             // Set the main stack pointer to the applications MSP.
        "ldr   r0, [%0, #0x04]\t\n"     // Load Reset handler into R0.

        "movs  r4, #0xFF\t\n"           // Move ones to R4.
        "sxtb  r4, r4\t\n"              // Sign extend R4 to obtain 0xFFFFFFFF instead of 0xFF.

        "mrs   r5, IPSR\t\n"            // Load IPSR to R5 to check for handler or thread mode.
        "cmp   r5, #0x00\t\n"           // Compare, if 0 then we are in thread mode and can continue to reset handler of bootloader.

        "mov   lr, r4\t\n"              // Clear the link register and set to ones to ensure no return.
        "bx    r0\t\n"                  // Branch to reset handler of bootloader.

        "isr_abort:  \t\n"

        "mov   r5, r4\t\n"              // Fill with ones before jumping to reset handling. Will be popped as LR when exiting ISR. Ensures no return to application.
        "mov   r6, r0\t\n"              // Move address of reset handler to R6. Will be popped as PC when exiting ISR. Ensures the reset handler will be executed when exist ISR.
        "movs  r7, #0x21\t\n"           // Move MSB reset value of xPSR to R7. Will be popped as xPSR when exiting ISR. xPSR is 0x21000000 thus MSB is 0x21.
        "rev   r7, r7\t\n"              // Reverse byte order to put 0x21 as MSB.
        "push  {r4-r7}\t\n"             // Push everything to new stack to allow interrupt handler to fetch it on exiting the ISR.

        "movs  r4, #0x00\t\n"           // Fill with zeros before jumping to reset handling. We be popped as R0 when exiting ISR (Cleaning up of the registers).
        "movs  r5, #0x00\t\n"           // Fill with zeros before jumping to reset handling. We be popped as R1 when exiting ISR (Cleaning up of the registers).
        "movs  r6, #0x00\t\n"           // Fill with zeros before jumping to reset handling. We be popped as R2 when exiting ISR (Cleaning up of the registers).
        "movs  r7, #0x00\t\n"           // Fill with zeros before jumping to reset handling. We be popped as R3 when exiting ISR (Cleaning up of the registers).
        "push  {r4-r7}\t\n"             // Push zeros (R4-R7) to stack to prepare for exiting the interrupt routine.

        "movs  r0, #0xF9\t\n"           // Move the execution return command into register, 0xFFFFFFF9.
        "sxtb  r0, r0\t\n"              // Sign extend R0 to obtain 0xFFFFFFF9 instead of 0xF9.
        "bx    r0\t\n"                  // No return - Handler mode will be exited. Stack will be popped and execution will continue in reset handler initializing other application.
        ".align\t\n"
        :: "r" (start_addr)             // Argument list for the gcc assembly. start_addr is %0.
        :  "r0", "r4", "r5", "r6", "r7" // List of register maintained manually.
    );
}

static void bootloader_util_app_start_local(uint32_t start_addr)
{
    bootloader_util_reset_local(start_addr);
}


int main(void) {
  trace_init();
      ITM_SendString(0, "\r\n" __DATE__ "\r\n" __TIME__ "\r\n");
	if (nrf_wdt_started()) {
		// WDT is already running; feed it
		for(uint32_t i = 0; i < NRF_WDT_CHANNEL_NUMBER; i++) {
			nrf_wdt_rr_register_t reload_reg = (nrf_wdt_rr_register_t)(NRF_WDT_RR0 + i);
			if (nrf_wdt_reload_request_is_enabled(reload_reg)) {
				nrf_wdt_reload_request_set(reload_reg);
			}
		}
	} else {
		// WDT isn't running yet, so start it. This is done so that we can rely on it
		// always running (we expect to come in here with it running, so this is only
		// to be safe)

		// nrf_drv_wdt_init(NULL, on_wdt_timeout);
		nrf_wdt_behaviour_set(NRF_WDT_BEHAVIOUR_RUN_SLEEP);
		nrf_wdt_reload_value_set(0xfffffffe);

		NVIC_SetPriority(WDT_IRQn, APP_IRQ_PRIORITY_HIGH);
		NVIC_ClearPendingIRQ(WDT_IRQn);
		NVIC_EnableIRQ(WDT_IRQn);

		// nrf_drv_wdt_channel_id wdt_channel;
		// nrf_drv_wdt_channel_alloc(&wdt_channel);
		nrf_wdt_reload_request_enable(NRF_WDT_RR0);

		// nrf_drv_wdt_enable();
		nrf_wdt_int_enable(NRF_WDT_INT_TIMEOUT_MASK);
		nrf_wdt_task_trigger(NRF_WDT_TASK_START);

		// nrf_drv_wdt_feed();
		nrf_wdt_reload_request_set(NRF_WDT_RR0);
	}
	PayloadDescriptor_t* pPayloadDescriptor = (PayloadDescriptor_t*)&_binary__build_obj_payload_descriptor_bin_start;

	// Erase region and copy the megadfu-finalise to immediately before the bootloader (pstorage pages??)
	// Needs to be cleaned up by the real application on first-run before pstorage or BLE are initialized.
	for (uint32_t eraseAddress=(uint32_t)pPayloadDescriptor->finalize_start; eraseAddress<(uint32_t)pPayloadDescriptor->bl_start; eraseAddress+=0x1000) {
		prx_nvmc_page_erase(eraseAddress);
	}
	unsigned char* pFinalizeStart = (unsigned char*)&_binary__build_obj_payload_finalize_bin_start;
	unsigned char* pFinalizeEnd = (unsigned char*)&_binary__build_obj_payload_finalize_bin_end;
	unsigned int finalizeSize = (uint32_t)(pFinalizeEnd - pFinalizeStart);

	prx_nvmc_write_words((uint32_t)pPayloadDescriptor->finalize_start, (uint32_t*)pFinalizeStart, finalizeSize / sizeof(uint32_t));
	// Erase the UICR so that we can be sure that storing the payload addresses in UICR->Customer is safe
	// NOTE: This will obliterate the NRFFW[0], NRFFW[1], PSELRESET[0], PSELRESET[1], and NFCPINS values
	prx_nvmc_page_erase((uint32_t)NRF_UICR);

	// Restore the appropriate settings
	prx_nvmc_write_word((uint32_t)&(NRF_UICR->NRFFW[0]), (uint32_t)pPayloadDescriptor->bl_start);
	prx_nvmc_write_word((uint32_t)&(NRF_UICR->NRFFW[1]), 		0x7E000ul);	// Always FLASH_SIZE-2*CODE_PAGE_SIZE
	prx_nvmc_write_word((uint32_t)&(NRF_UICR->NFCPINS), 		0xFFFFFFFEul);	// NFC pins disabled
	
	// Store all payload addresses in UICR->Customer
	prx_nvmc_write_word((uint32_t)&(NRF_UICR->CUSTOMER[24]), (uint32_t)&_binary__build_obj_payload_descriptor_bin_start);
	prx_nvmc_write_word((uint32_t)&(NRF_UICR->CUSTOMER[25]), (uint32_t)&_binary__build_obj_payload_descriptor_bin_end);
	prx_nvmc_write_word((uint32_t)&(NRF_UICR->CUSTOMER[26]), (uint32_t)&_binary__build_obj_payload_softdevice_lz4_start);
	prx_nvmc_write_word((uint32_t)&(NRF_UICR->CUSTOMER[27]), (uint32_t)&_binary__build_obj_payload_softdevice_lz4_end);
	prx_nvmc_write_word((uint32_t)&(NRF_UICR->CUSTOMER[28]), (uint32_t)&_binary__build_obj_payload_bootloader_lz4_start);
	prx_nvmc_write_word((uint32_t)&(NRF_UICR->CUSTOMER[29]), (uint32_t)&_binary__build_obj_payload_bootloader_lz4_end);
	prx_nvmc_write_word((uint32_t)&(NRF_UICR->CUSTOMER[30]), (uint32_t)&_binary__build_obj_payload_settings_lz4_start);
	prx_nvmc_write_word((uint32_t)&(NRF_UICR->CUSTOMER[31]), (uint32_t)&_binary__build_obj_payload_settings_lz4_end);

	trace_init();

	for(uint32_t i = 0; i < NRF_WDT_CHANNEL_NUMBER; i++) {
	  nrf_wdt_rr_register_t reload_reg = (nrf_wdt_rr_register_t)(NRF_WDT_RR0 + i);
	  if (nrf_wdt_reload_request_is_enabled(reload_reg)) {
	    nrf_wdt_reload_request_set(reload_reg);
	  }
	}

	NRF_POWER->RESETREAS = 0xffffffff;
	
	// Jump to the finalize application
	bootloader_util_app_start_local((uint32_t)pPayloadDescriptor->finalize_start);

	return 0;
}
