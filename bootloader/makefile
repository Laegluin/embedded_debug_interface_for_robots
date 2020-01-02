STM_CUBE_DIR := ../vendor/stm32_cube_f7_1.15.0

CC := arm-none-eabi-gcc
SIZE := arm-none-eabi-size
OBJCOPY := arm-none-eabi-objcopy

INCLUDE_FLAGS := \
	-Isrc \
	-Isrc/config \
	-I$(STM_CUBE_DIR)/Drivers/STM32F7xx_HAL_Driver/Inc \
	-I$(STM_CUBE_DIR)/Drivers/BSP/STM32F7508-Discovery \
	-I$(STM_CUBE_DIR)/Middlewares/ST/STM32_USB_Device_Library/Core/Inc \
	-I$(STM_CUBE_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc \
	-I$(STM_CUBE_DIR)/Drivers/CMSIS/Device/ST/STM32F7xx/Include \
	-I$(STM_CUBE_DIR)/Drivers/CMSIS/Core/Include

ARCH_FLAGS := -mthumb -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard
CFLAGS := $(INCLUDE_FLAGS) $(ARCH_FLAGS) -std=c99 -g -Wall -Wextra -O3
LDFLAGS := $(ARCH_FLAGS) -flto

TARGET_DIR := target
dep_dir := $(TARGET_DIR)/deps
object_dir := $(TARGET_DIR)/obj


objects := $(addprefix $(object_dir)/,$(addsuffix .o,$(basename $(notdir $(wildcard src/*.s src/*.c src/config/*.c)))))

vendor_objects := $(addprefix $(object_dir)/,\
	stm32f7xx_hal.o \
	stm32f7xx_hal_cortex.o \
	stm32f7xx_hal_gpio.o \
	stm32f7xx_hal_rcc.o \
	stm32f7xx_hal_pwr.o \
	stm32f7xx_hal_pwr_ex.o \
	stm32f7xx_hal_rcc_ex.o \
	stm32f7xx_hal_pcd.o \
	stm32f7xx_hal_pcd_ex.o \
	stm32f7xx_ll_usb.o \
	usbd_core.o \
	usbd_ctlreq.o \
	usbd_ioreq.o \
	usbd_cdc.o \
)


build: $(TARGET_DIR)/bootloader.bin

format:
	@clang-format -style=file -i src/*.c src/*.h

clean:
	@rm -rf $(TARGET_DIR)

$(TARGET_DIR) $(object_dir) $(dep_dir):
	@mkdir -p $@

# project
$(object_dir)/%.o: src/%.s | $(object_dir)
	@$(CC) -c $< -o $@

$(object_dir)/%.o: src/%.c | $(object_dir) $(dep_dir)
	@$(CC) $(CFLAGS) -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

$(object_dir)/%.o: src/config/%.c | $(object_dir) $(dep_dir)
	@$(CC) $(CFLAGS) -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

# libraries
$(object_dir)/%.o: $(STM_CUBE_DIR)/Drivers/STM32F7xx_HAL_Driver/Src/%.c | $(object_dir) $(dep_dir)
	@$(CC) $(CFLAGS) -Wno-unused-parameter -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

$(object_dir)/%.o: $(STM_CUBE_DIR)/Drivers/BSP/STM32F7508-Discovery/%.c | $(object_dir) $(dep_dir)
	@$(CC) $(CFLAGS) -Wno-unused-parameter -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

$(object_dir)/%.o: $(STM_CUBE_DIR)/Middlewares/ST/STM32_USB_Device_Library/Core/Src/%.c | $(object_dir) $(dep_dir)
	@$(CC) $(CFLAGS) -Wno-unused-parameter -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

$(object_dir)/%.o: $(STM_CUBE_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/%.c | $(object_dir) $(dep_dir)
	@$(CC) $(CFLAGS) -Wno-unused-parameter -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

# bootloader binary
$(TARGET_DIR)/bootloader.elf: $(objects) $(vendor_objects) src/linker.ld | $(TARGET_DIR)
	@$(CC) $(LDFLAGS) --specs=rdimon.specs -Tsrc/linker.ld -Wl,--gc-sections -o $@ $(filter %.o,$^)

$(TARGET_DIR)/bootloader.bin: $(TARGET_DIR)/bootloader.elf
	@$(SIZE) $<
	@$(OBJCOPY) -O binary $< $@

.PHONY: build format clean


include $(wildcard $(dep_dir)/*.d)
