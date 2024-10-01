PROJECT_NAME := megadfu

export OUTPUT_FILENAME
#MAKEFILE_NAME := $(CURDIR)/$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))
MAKEFILE_NAME := $(MAKEFILE_LIST)
MAKEFILE_DIR := $(dir $(MAKEFILE_NAME) ) 

NRF_SDK_PATH = /mnt/c/dev/nRF5_SDK_11.0.0_89a8197
TEMPLATE_PATH = $(NRF_SDK_PATH)/components/toolchain/gcc
ifeq ($(OS),Windows_NT)
include $(TEMPLATE_PATH)/Makefile.windows
else
include $(TEMPLATE_PATH)/Makefile.posix
endif

MK := mkdir
RM := rm -rf
CP := cp

#echo suspend
ifeq ("$(VERBOSE)","1")
NO_ECHO := 
else
NO_ECHO := @
endif

# Toolchain commands
CC              := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-gcc'
AS              := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-as'
AR              := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-ar' -r
LD              := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-ld'
NM              := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-nm'
OBJDUMP         := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-objdump'
OBJCOPY         := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-objcopy'
SIZE            := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-size'
LZ4				:= $(abspath lz4)
MERGEHEX		:= $(abspath mergehex)

# Function for removing duplicates in a list
remduplicates = $(strip $(if $1,$(firstword $1) $(call remduplicates,$(filter-out $(firstword $1),$1))))

# Payloads to convert to embed in the binary
FINALIZE_PAYLOAD_SOURCE_FILE  	= $(abspath ../megadfu-finalize/_build/megadfu-finalize.bin)
# Must match megadfu-finalize linker script
FINALIZE_START                	= 0x77000
SOFTDEVICE_PAYLOAD_SOURCE_FILE  = $(abspath s132_nrf52_7.2.0_softdevice.hex)
SOFTDEVICE_START                = 0x0
BOOTLOADER_PAYLOAD_SOURCE_FILE  = $(abspath sd_bl.bin)
# Must match bootloader linker script
BOOTLOADER_START                = 0x78000
SETTINGS_PAYLOAD_SOURCE_FILE    = $(abspath bootloader_setting.hex)
SETTINGS_START                  = 0x7E000
#APPLICATION_PAYLOAD_SOURCE_FILE = $(abspath firmware.hex)
# Must match SoftDevice size
APPLICATION_START               = 0x26000

# Source common to all targets
C_SOURCE_FILES  = $(abspath main.c)
C_SOURCE_FILES += $(abspath itm_messages.c)
C_SOURCE_FILES += $(abspath prx_nvmc.c)
C_SOURCE_FILES += $(abspath $(NRF_SDK_PATH)/components/libraries/bootloader_dfu/bootloader_util.c)
#C_SOURCE_FILES += $(abspath $(NRF_SDK_PATH)/components/libraries/util/app_util_platform.c)
C_SOURCE_FILES += $(abspath $(NRF_SDK_PATH)/components/drivers_nrf/common/nrf_drv_common.c)
#C_SOURCE_FILES += $(abspath $(NRF_SDK_PATH)/components/drivers_nrf/wdt/nrf_drv_wdt.c)
C_SOURCE_FILES += $(abspath $(NRF_SDK_PATH)/components/toolchain/system_nrf52.c)
C_SOURCE_FILES += $(abspath $(NRF_SDK_PATH)/components/drivers_nrf/uart/nrf_drv_uart.c)

# Assembly files common to all targets
ASM_SOURCE_FILES = $(abspath $(NRF_SDK_PATH)/components/toolchain/gcc/gcc_startup_nrf52.s)

# Includes common to all targets
INC_PATHS  = -I$(abspath .)
INC_PATHS += -I$(abspath $(NRF_SDK_PATH)/components/libraries/bootloader_dfu)
INC_PATHS += -I$(abspath $(NRF_SDK_PATH)/components/drivers_nrf/config)
INC_PATHS += -I$(abspath $(NRF_SDK_PATH)/components/libraries/util)
INC_PATHS += -I$(abspath $(NRF_SDK_PATH)/components/device)
INC_PATHS += -I$(abspath $(NRF_SDK_PATH)/components/toolchain/CMSIS/Include)
INC_PATHS += -I$(abspath $(NRF_SDK_PATH)/components/drivers_nrf/hal)
INC_PATHS += -I$(abspath $(NRF_SDK_PATH)/components/drivers_nrf/nrf_soc_nosd)
#INC_PATHS += -I$(abspath $(NRF_SDK_PATH)/components/drivers_nrf/wdt)
INC_PATHS += -I$(abspath $(NRF_SDK_PATH)/components/toolchain/gcc)
INC_PATHS += -I$(abspath $(NRF_SDK_PATH)/components/toolchain)
INC_PATHS += -I$(abspath $(NRF_SDK_PATH)/components/drivers_nrf/common)
INC_PATHS += -I$(abspath $(NRF_SDK_PATH)/components/drivers_nrf/uart)

OBJECT_DIRECTORY = _build/obj
LISTING_DIRECTORY = _build
OUTPUT_BINARY_DIRECTORY = _build

# Sorting removes duplicates
BUILD_DIRECTORIES := $(sort $(OBJECT_DIRECTORY) $(OUTPUT_BINARY_DIRECTORY) $(LISTING_DIRECTORY) )

# Flags common to all targets
CFLAGS  = -DNRF52
CFLAGS += -DNRF52_PAN_12
CFLAGS += -DNRF52_PAN_15
CFLAGS += -DNRF52_PAN_58
CFLAGS += -DNRF52_PAN_55
CFLAGS += -DNRF52_PAN_54
CFLAGS += -DNRF52_PAN_31
CFLAGS += -DNRF52_PAN_30
CFLAGS += -DNRF52_PAN_51
CFLAGS += -DNRF52_PAN_36
CFLAGS += -DNRF52_PAN_53
CFLAGS += -DCONFIG_GPIO_AS_PINRESET
CFLAGS += -DCONFIG_NFCT_PINS_AS_GPIOS
CFLAGS += -DSWI_DISABLE0
CFLAGS += -DNRF52_PAN_20
CFLAGS += -DNRF52_PAN_64
CFLAGS += -DNRF52_PAN_62
CFLAGS += -DNRF52_PAN_63
CFLAGS += -mcpu=cortex-m4
CFLAGS += -mthumb -mabi=aapcs --std=gnu99
CFLAGS += -Wall -Werror -Os -g3
CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# keep every function in separate section. This will allow linker to dump unused functions
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin --short-enums 
# keep every function in separate section. This will allow linker to dump unused functions
LDFLAGS += -Xlinker -Map=$(LISTING_DIRECTORY)/$(OUTPUT_FILENAME).map
LDFLAGS += -mthumb -mabi=aapcs -L $(TEMPLATE_PATH) -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m4
LDFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# let linker to dump unused sections
LDFLAGS += -Wl,--gc-sections
# use newlib in nano version
LDFLAGS += --specs=nano.specs -lc -lnosys

# Assembler flags
ASMFLAGS += -x assembler-with-cpp
ASMFLAGS += -DNRF52
ASMFLAGS += -DNRF52_PAN_12
ASMFLAGS += -DNRF52_PAN_15
ASMFLAGS += -DNRF52_PAN_58
ASMFLAGS += -DNRF52_PAN_55
ASMFLAGS += -DNRF52_PAN_54
ASMFLAGS += -DNRF52_PAN_31
ASMFLAGS += -DNRF52_PAN_30
ASMFLAGS += -DNRF52_PAN_51
ASMFLAGS += -DNRF52_PAN_36
ASMFLAGS += -DNRF52_PAN_53
ASMFLAGS += -DCONFIG_GPIO_AS_PINRESET
ASMFLAGS += -DCONFIG_NFCT_PINS_AS_GPIOS
ASMFLAGS += -DSWI_DISABLE0
ASMFLAGS += -DNRF52_PAN_20
ASMFLAGS += -DNRF52_PAN_64
ASMFLAGS += -DNRF52_PAN_62
ASMFLAGS += -DNRF52_PAN_63

#default target - first one defined
default: clean megadfu

#building all targets
all: clean
	$(NO_ECHO)$(MAKE) -f $(MAKEFILE_NAME) -C $(MAKEFILE_DIR) -e cleanobj
	$(NO_ECHO)$(MAKE) -f $(MAKEFILE_NAME) -C $(MAKEFILE_DIR) -e megadfu

#target for printing all targets
help:
	@echo following targets are available:
	@echo 	megadfu
	@echo 	flash_softdevice

PAYLOAD_OBJECTS  = $(OBJECT_DIRECTORY)/payload_finalize.o
PAYLOAD_OBJECTS += $(OBJECT_DIRECTORY)/payload_softdevice.o
PAYLOAD_OBJECTS += $(OBJECT_DIRECTORY)/payload_bootloader.o
PAYLOAD_OBJECTS += $(OBJECT_DIRECTORY)/payload_settings.o
# PAYLOAD_OBJECTS += $(OBJECT_DIRECTORY)/payload_application.o
PAYLOAD_OBJECTS += $(OBJECT_DIRECTORY)/payload_descriptor.o

C_SOURCE_FILE_NAMES = $(notdir $(C_SOURCE_FILES))
C_PATHS = $(call remduplicates, $(dir $(C_SOURCE_FILES) ) )
C_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(C_SOURCE_FILE_NAMES:.c=.o) )

ASM_SOURCE_FILE_NAMES = $(notdir $(ASM_SOURCE_FILES))
ASM_PATHS = $(call remduplicates, $(dir $(ASM_SOURCE_FILES) ))
ASM_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(ASM_SOURCE_FILE_NAMES:.s=.o) )

vpath %.c $(C_PATHS)
vpath %.s $(ASM_PATHS)

OBJECTS = $(C_OBJECTS) $(ASM_OBJECTS) $(PAYLOAD_OBJECTS)

megadfu: OUTPUT_FILENAME := megadfu
megadfu: LINKER_SCRIPT=megadfu-sdk11-dfu.ld

megadfu: $(BUILD_DIRECTORIES) $(OBJECTS)
	@echo Linking target: $(OUTPUT_FILENAME).out
	$(NO_ECHO)$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -lm -o $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
	$(NO_ECHO)$(MAKE) -f $(MAKEFILE_NAME) -C $(MAKEFILE_DIR) -e finalize

## Create build directories
$(BUILD_DIRECTORIES):
	$(NO_ECHO)$(MK) $@

# Create objects from payload files
$(OBJECT_DIRECTORY)/payload_descriptor.bin:
	@echo Payload file 1/2: $(notdir $<) 
	$(NO_ECHO)python3 struct_builder.py -o $@ -v $(FINALIZE_START) $(SOFTDEVICE_START) $(BOOTLOADER_START) $(SETTINGS_START) $(APPLICATION_START)

$(OBJECT_DIRECTORY)/payload_softdevice.bin: $(SOFTDEVICE_PAYLOAD_SOURCE_FILE)
	@echo Payload file 1/3: $(notdir $<) 
	$(NO_ECHO)$(OBJCOPY) -I ihex -O binary -B arm --gap-fill 0xFF $< $@

$(OBJECT_DIRECTORY)/payload_bootloader.bin: $(BOOTLOADER_PAYLOAD_SOURCE_FILE) $(OBJECT_DIRECTORY)/payload_softdevice.bin
	@echo Payload file 1/3: $(notdir $<) 
	$(NO_ECHO)python3 binsplit.py -i $< -partition $(OBJECT_DIRECTORY)/payload_softdevice.bin -offset=-0x1000 -pB $@

$(OBJECT_DIRECTORY)/payload_settings.bin: $(SETTINGS_PAYLOAD_SOURCE_FILE)
	@echo Payload file 1/3: $(notdir $<) 
	$(NO_ECHO)$(OBJCOPY) -I ihex -O binary -B arm --gap-fill 0xFF $< $@

# $(OBJECT_DIRECTORY)/payload_application.bin: $(APPLICATION_PAYLOAD_SOURCE_FILE)
# 	@echo Payload file 1/3: $(notdir $<) 
# 	$(NO_ECHO)$(OBJCOPY) -I ihex -O binary -B arm --gap-fill 0xFF $< $@

$(OBJECT_DIRECTORY)/payload_softdevice.lz4: $(OBJECT_DIRECTORY)/payload_softdevice.bin
	@echo Payload file 2/3: $(notdir $<) 
	$(NO_ECHO)$(LZ4) $< $@

$(OBJECT_DIRECTORY)/payload_bootloader.lz4: $(OBJECT_DIRECTORY)/payload_bootloader.bin
	@echo Payload file 2/3: $(notdir $<) 
	$(NO_ECHO)$(LZ4) $< $@

$(OBJECT_DIRECTORY)/payload_settings.lz4: $(OBJECT_DIRECTORY)/payload_settings.bin
	@echo Payload file 2/3: $(notdir $<) 
	$(NO_ECHO)$(LZ4) $< $@

# $(OBJECT_DIRECTORY)/payload_application.lz4: $(OBJECT_DIRECTORY)/payload_application.bin
# 	@echo Payload file 2/3: $(notdir $<) 
# 	$(NO_ECHO)$(LZ4) $< $@

$(OBJECT_DIRECTORY)/payload_descriptor.o: $(OBJECT_DIRECTORY)/payload_descriptor.bin
	@echo Payload file 2/2: $(notdir $<) 
	$(NO_ECHO)$(OBJCOPY) -I binary -O elf32-littlearm -B arm --rename-section .data=.payload_descriptor,alloc,load,readonly,data,contents $< $@

$(OBJECT_DIRECTORY)/payload_finalize.bin: $(FINALIZE_PAYLOAD_SOURCE_FILE)
	@echo Payload file 1/2: $(notdir $<) 
	$(NO_ECHO)$(CP) $< $@

$(OBJECT_DIRECTORY)/payload_finalize.o: $(OBJECT_DIRECTORY)/payload_finalize.bin
	@echo Payload file 2/2: $(notdir $<) 
	$(NO_ECHO)$(OBJCOPY) -I binary -O elf32-littlearm -B arm --rename-section .data=.payload_finalize,alloc,load,readonly,data,contents $< $@

$(OBJECT_DIRECTORY)/payload_softdevice.o: $(OBJECT_DIRECTORY)/payload_softdevice.lz4
	@echo Payload file 3/3: $(notdir $<) 
	$(NO_ECHO)$(OBJCOPY) -I binary -O elf32-littlearm -B arm --rename-section .data=.payload_softdevice,alloc,load,readonly,data,contents $< $@

$(OBJECT_DIRECTORY)/payload_bootloader.o: $(OBJECT_DIRECTORY)/payload_bootloader.lz4
	@echo Payload file 3/3: $(notdir $<) 
	$(NO_ECHO)$(OBJCOPY) -I binary -O elf32-littlearm -B arm --rename-section .data=.payload_bootloader,alloc,load,readonly,data,contents $< $@

$(OBJECT_DIRECTORY)/payload_settings.o: $(OBJECT_DIRECTORY)/payload_settings.lz4
	@echo Payload file 3/3: $(notdir $<) 
	$(NO_ECHO)$(OBJCOPY) -I binary -O elf32-littlearm -B arm --rename-section .data=.payload_settings,alloc,load,readonly,data,contents $< $@

# $(OBJECT_DIRECTORY)/payload_application.o: $(OBJECT_DIRECTORY)/payload_application.lz4
# 	@echo Payload file 3/3: $(notdir $<) 
# 	$(NO_ECHO)$(OBJCOPY) -I binary -O elf32-littlearm -B arm --rename-section .data=.payload_application,alloc,load,readonly,data,contents $< $@

#$(OBJECT_DIRECTORY)/%.o: %.dat
#	@echo Payload DAT file: $(notdir $<)
#	$(NO_ECHO)$(OBJCOPY) -I binary -O elf32-littlearm -B arm $< $@

#$(OBJECT_DIRECTORY)/%.o: %.bin
#	@echo Payload BIN file: $(notdir $<)
#	$(NO_ECHO)$(OBJCOPY) -I binary -O elf32-littlearm -B arm $< $@

#$(OBJECT_DIRECTORY)/%.o: %.hex
#	@echo Payload HEX file: $(notdir $<)
#	$(NO_ECHO)$(OBJCOPY) -I ihex -O elf32-littlearm -B arm --gap-fill 0xFF $< $@

# Create objects from C SRC files
$(OBJECT_DIRECTORY)/%.o: %.c
	@echo Compiling file: $(notdir $<)
	$(NO_ECHO)$(CC) $(CFLAGS) $(INC_PATHS) -c -o $@ $<

# Assemble files
$(OBJECT_DIRECTORY)/%.o: %.s
	@echo Assembly file: $(notdir $<)
	$(NO_ECHO)$(CC) $(ASMFLAGS) $(INC_PATHS) -c -o $@ $<

# Link
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out: $(BUILD_DIRECTORIES) $(OBJECTS)
	@echo Linking target: $(OUTPUT_FILENAME).out
	$(NO_ECHO)$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -lm -o $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
## Create binary .bin file from the .out file
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).bin: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
	@echo Preparing: $(OUTPUT_FILENAME).bin
	$(NO_ECHO)$(OBJCOPY) -O binary $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).bin

## Create binary .hex file from the .out file
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).hex: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
	@echo Preparing: $(OUTPUT_FILENAME).hex
	$(NO_ECHO)$(OBJCOPY) -O ihex $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).hex

finalize: genbin genhex echosize

genbin:
	@echo Preparing: $(OUTPUT_FILENAME).bin
	$(NO_ECHO)$(OBJCOPY) -O binary $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).bin

## Create binary .hex file from the .out file
genhex: 
	@echo Preparing: $(OUTPUT_FILENAME).hex
	$(NO_ECHO)$(OBJCOPY) -O ihex $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).hex

echosize:
	-@echo ''
	$(NO_ECHO)$(SIZE) $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
	-@echo ''

## Clean
clean:
	$(RM) $(BUILD_DIRECTORIES)

cleanobj:
	$(RM) $(BUILD_DIRECTORIES)/*.o

## Flash application
flash: megadfu
	@echo Flashing: $(OUTPUT_BINARY_DIRECTORY)/$<.hex
	nrfjprog --program $(OUTPUT_BINARY_DIRECTORY)/$<.hex -f nrf52  --sectorerase
	nrfjprog --reset -f nrf52

## Flash softdevice
flash_softdevice:
	@echo Flashing: s132_nrf52_2.0.0_softdevice.hex
	nrfjprog --program $(NRF_SDK_PATH)/components/softdevice/s132/hex/s132_nrf52_2.0.0_softdevice.hex -f nrf52 --chiperase
	nrfjprog --reset -f nrf52
