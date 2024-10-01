#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "app_util_platform.h"
#include "nrf_wdt.h"
#include "bootloader_util.h"

#include "prx_nvmc.h"

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

int main(void) {
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
		nrf_wdt_reload_value_set((20000 * 32768) / 1000);

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
        prx_nvmc_write_word((uint32_t)&(NRF_UICR->PSELRESET[0]),        0x15ul);        // Enabled on Pin 21
        prx_nvmc_write_word((uint32_t)&(NRF_UICR->PSELRESET[1]),        0x15ul);        // copy of [0]
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

	// Jump to the finalize application
	bootloader_util_app_start((uint32_t)pPayloadDescriptor->finalize_start);

	return 0;
}
