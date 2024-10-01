#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "nrf.h"

#include "prx_nvmc.h"
#include "LZ4.h"


typedef enum
{
	STREAM_OK,
	STREAM_ERROR_READ_PAST_END,
	STREAM_ERROR_DATA_REMAINING
} StreamDecompress_e;

typedef struct
{
	unsigned int* pStart;
	unsigned int* pCurr;

	unsigned char write_buf[3];
	unsigned char write_buf_size;
} StreamContext_t;

static void stream_writer(void* ctx, unsigned char val)
{
	StreamContext_t* p_stream_context = (StreamContext_t*)ctx;

	if (p_stream_context->write_buf_size < sizeof(p_stream_context->write_buf))
	{
		p_stream_context->write_buf[p_stream_context->write_buf_size++] = val;
		return;
	}

	unsigned char* pWB = p_stream_context->write_buf;
	uint32_t word_val = (val << 24) | (pWB[2] << 16) | (pWB[1] << 8) | pWB[0];

	prx_nvmc_write_word((uint32_t)p_stream_context->pCurr, word_val);
	p_stream_context->pCurr++;

	p_stream_context->write_buf_size = 0;
}

static unsigned char stream_reader(void* ctx, unsigned int offset_from_start)
{
	StreamContext_t* p_stream_context = (StreamContext_t*)ctx;

	unsigned char val;

	uint32_t word = (offset_from_start & ~(sizeof(uint32_t) - 1)) / sizeof(uint32_t);
	uint32_t word_byte = offset_from_start & (sizeof(uint32_t) - 1);
	if ((int)word == (p_stream_context->pCurr - p_stream_context->pStart))
	{
		// Still in the write buffer
		val = p_stream_context->write_buf[word_byte];
	}
	else
	{
		uint32_t word_val = p_stream_context->pStart[word];
		val = (unsigned char)((word_val >> (word_byte * 8)) & 0xFF);
	}

	return val;
}

static StreamDecompress_e stream_decompress(
	LZ4_stream_write stream_write,
	LZ4_stream_read stream_read,
	unsigned char* compressedData, int compressedSize,
	unsigned char* decompressedData, unsigned int* decompressedSize)
{
	StreamContext_t stream_context;
	memset(&stream_context, 0, sizeof(stream_context));
	stream_context.pStart = (unsigned int*)decompressedData;
	stream_context.pCurr = (unsigned int*)decompressedData;
	
	*decompressedSize = 0;

	unsigned char* pStart = compressedData;
	unsigned char* pEnd = compressedData + compressedSize;
	int compressed_size_remaining = compressedSize;
	while (pStart < pEnd) {
		unsigned int decompressed_chunk_size = 0;
		LZ4_stream_decode(
			&stream_context, stream_write, stream_read,
			pStart, &compressed_size_remaining,
			*decompressedSize, &decompressed_chunk_size);
		*decompressedSize += decompressed_chunk_size;

		pStart = compressedData + (compressedSize - compressed_size_remaining);
	}

	if (stream_context.write_buf_size != 0)
	{
		return STREAM_ERROR_DATA_REMAINING;
	}

	if (pStart != pEnd)
	{
		return STREAM_ERROR_READ_PAST_END;
	}

	return STREAM_OK;
}


typedef struct {
	unsigned char* finalize_start;
	unsigned char* sd_start;
	unsigned char* bl_start;
	unsigned char* settings_start;
	unsigned char* app_start;
} PayloadDescriptor_t;


// Never inline this function as we need the PayloadDescriptor_t to be on the stack
// as the original will be obliterated at some point by the erase/write ops
void __attribute__ ((noinline)) perform_finalize(PayloadDescriptor_t payloadDescriptor) {
	uint32_t payload_softdevice_lz4_start	= NRF_UICR->CUSTOMER[26];
	uint32_t payload_softdevice_lz4_end		= NRF_UICR->CUSTOMER[27];
	uint32_t payload_bootloader_lz4_start	= NRF_UICR->CUSTOMER[28];
	uint32_t payload_bootloader_lz4_end		= NRF_UICR->CUSTOMER[29];
	uint32_t payload_settings_lz4_start		= NRF_UICR->CUSTOMER[30];
	uint32_t payload_settings_lz4_end		= NRF_UICR->CUSTOMER[31];
	// uint32_t payload_application_lz4_start	= NRF_UICR->CUSTOMER[30];
	// uint32_t payload_application_lz4_end	= NRF_UICR->CUSTOMER[31];

	StreamDecompress_e stream_result;

	// Bootloader
	for (uint32_t eraseAddress=(uint32_t)payloadDescriptor.bl_start; eraseAddress<(uint32_t)payloadDescriptor.settings_start; eraseAddress+=0x1000) {
		prx_nvmc_page_erase(eraseAddress);
	}
	unsigned char* pBLCompressedStart = (unsigned char*)payload_bootloader_lz4_start;
	unsigned char* pBLCompressedEnd = (unsigned char*)payload_bootloader_lz4_end;
	unsigned int blCompressedSize = (uint32_t)(pBLCompressedEnd - pBLCompressedStart);
	unsigned int blSize = 0;
	stream_result = stream_decompress(
		stream_writer, stream_reader,
		pBLCompressedStart, blCompressedSize, 
		payloadDescriptor.bl_start, &blSize);
	if (stream_result != STREAM_OK) {
	}
	
	// Bootloader settings and MBR settings pages
	for (uint32_t eraseAddress=(uint32_t)payloadDescriptor.settings_start; eraseAddress<0x80000; eraseAddress+=0x1000) {
		prx_nvmc_page_erase(eraseAddress);
	}
	unsigned char* pSettingsCompressedStart = (unsigned char*)payload_settings_lz4_start;
	unsigned char* pSettingsCompressedEnd = (unsigned char*)payload_settings_lz4_end;
	unsigned int settingsCompressedSize = (uint32_t)(pSettingsCompressedEnd - pSettingsCompressedStart);
	unsigned int settingsSize = 0;
	stream_result = stream_decompress(
		stream_writer, stream_reader,
		pSettingsCompressedStart, settingsCompressedSize, 
		payloadDescriptor.settings_start, &settingsSize);
	if (stream_result != STREAM_OK) {
	}

	// SoftDevice
	for (uint32_t eraseAddress=0x0; eraseAddress<(uint32_t)payloadDescriptor.app_start; eraseAddress+=0x1000) {
		prx_nvmc_page_erase(eraseAddress);
	}
	unsigned char* pSDCompressedStart = (unsigned char*)payload_softdevice_lz4_start;
	unsigned char* pSDCompressedEnd = (unsigned char*)payload_softdevice_lz4_end;
	unsigned int sdCompressedSize = (uint32_t)(pSDCompressedEnd - pSDCompressedStart);
	unsigned int sdSize = 0;
	stream_result = stream_decompress(
		stream_writer, stream_reader,
		pSDCompressedStart, sdCompressedSize, 
		payloadDescriptor.sd_start, &sdSize);
	if (stream_result != STREAM_OK) {
	}

	// Application disabled - erase application area instead
	for (uint32_t eraseAddress=(uint32_t)payloadDescriptor.app_start; eraseAddress<(uint32_t)payloadDescriptor.finalize_start; eraseAddress+=0x1000) {
		prx_nvmc_page_erase(eraseAddress);
	}
}

//
// BEWARE.  There is only 4k of flash allocated for this application.  It's easy to exceed that when adding debug code.  Default size is 0x00000EFC
//

int main(void) {
	uint32_t payload_descriptor_bin_start	= NRF_UICR->CUSTOMER[24];
	PayloadDescriptor_t* pPayloadDescriptor = (PayloadDescriptor_t*)payload_descriptor_bin_start;
	perform_finalize(*pPayloadDescriptor);
	
	// DONE!

	// Wait for WDT, as it will be running with the OEM bootloader's configuration
	// and the only way to stop/clear it is to reset due to WDT.
	while (1) {
		// Do nothing
	}

	return 0;
}
