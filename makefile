SERIAL ?= /dev/ttyS4

STM_CUBE_DIR := vendor/stm32_cube_f7_1.15.0

CXX := arm-none-eabi-g++
SIZE := arm-none-eabi-size
OBJCOPY := arm-none-eabi-objcopy

INCLUDE_FLAGS := \
	-Isrc \
	-Isrc/config \
	-Iinclude \
	-I$(STM_CUBE_DIR)/Middlewares/ST/STemWin/inc \
	-I$(STM_CUBE_DIR)/Drivers/STM32F7xx_HAL_Driver/Inc \
	-I$(STM_CUBE_DIR)/Drivers/CMSIS/Device/ST/STM32F7xx/Include \
	-I$(STM_CUBE_DIR)/Drivers/CMSIS/Core/Include

ARCH_FLAGS := -mthumb -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard
CXXFLAGS := $(INCLUDE_FLAGS) $(ARCH_FLAGS) -std=c++14 -Wall -Wextra -O3
LDFLAGS := $(ARCH_FLAGS) -flto
TEST_CXX := g++
TEST_CXXFLAGS := $(INCLUDE_FLAGS) -std=c++14 -g -Wall -Wextra -DTEST
TEST_LDFLAGS :=

TARGET_DIR := target
dep_dir := $(TARGET_DIR)/deps
object_dir := $(TARGET_DIR)/obj
test_dep_dir := $(dep_dir)/test
test_object_dir := $(object_dir)/test


objects := $(addprefix $(object_dir)/,\
	startup.o \
	init.o \
	system_stm32f7xx.o \
	main.o \
	app.o \
	GUI_X.o \
	LCDConf.o \
	GUIConf.o \
	parser.o \
	buffer.o \
	control_table.o \
	mx64_control_table.o \
	mx106_control_table.o \
)

test_objects := $(addprefix $(test_object_dir)/,test.o parser.o buffer.o control_table.o mx64_control_table.o mx106_control_table.o)

vendor_objects := $(addprefix $(object_dir)/,\
	stm32f7xx_hal.o \
	stm32f7xx_hal_cortex.o \
	stm32f7xx_hal_dma.o \
	stm32f7xx_hal_rcc.o \
	stm32f7xx_hal_gpio.o \
	stm32f7xx_hal_uart.o \
	stm32f7xx_hal_ltdc.o \
	stm32f7xx_hal_sdram.o \
	stm32f7xx_ll_fmc.o \
	stm32f7xx_hal_pwr.o \
	stm32f7xx_hal_rcc_ex.o \
)

archives := $(STM_CUBE_DIR)/Middlewares/ST/STemWin/Lib/STemWin_CM7_wc32_ot_ARGB.a


build: $(TARGET_DIR)/firmware.bin

flash: $(TARGET_DIR)/firmware.bin $(TARGET_DIR)/send_command
	@$(abspath $(TARGET_DIR)/send_command) flash $(SERIAL) $<

start: $(TARGET_DIR)/send_command
	@$(abspath $(TARGET_DIR)/send_command) start $(SERIAL)

test: $(TARGET_DIR)/test
	@$(abspath $<)

format:
	@clang-format -style=file -i src/*.cpp src/*.h

clean:
	@rm -rf $(TARGET_DIR)

$(TARGET_DIR) $(object_dir) $(test_object_dir) $(dep_dir) $(test_dep_dir):
	@mkdir -p $@

# test objects
$(test_object_dir)/%.o: src/%.cpp | $(test_object_dir) $(test_dep_dir)
	@$(TEST_CXX) $(TEST_CXXFLAGS) -MT $@ -MD -MP -MF $(test_dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

# project objects
$(object_dir)/%.o: src/%.s | $(object_dir)
	@$(CXX) -c $< -o $@

$(object_dir)/%.o: src/%.cpp | $(object_dir) $(dep_dir)
	@$(CXX) $(CXXFLAGS) -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

$(object_dir)/%.o: src/config/%.c | $(object_dir) $(dep_dir)
	@$(CXX) $(CXXFLAGS) -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

# vendor objects
$(object_dir)/%.o: $(STM_CUBE_DIR)/Drivers/STM32F7xx_HAL_Driver/Src/%.c | $(object_dir) $(dep_dir)
	@$(CXX) $(CXXFLAGS) -Wno-unused-parameter -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

# actual binary for mcu
$(TARGET_DIR)/firmware.elf: $(objects) $(vendor_objects) $(archives) src/linker.ld | $(TARGET_DIR)
	@$(CXX) $(LDFLAGS) --specs=rdimon.specs -Tsrc/linker.ld -Wl,--gc-sections -o $@ $(filter %.o %.a,$^)

$(TARGET_DIR)/firmware.bin: $(TARGET_DIR)/firmware.elf
	@$(SIZE) $<
	@$(OBJCOPY) -O binary $< $@

# test binary
$(TARGET_DIR)/test: $(test_objects) | $(TARGET_DIR)
	@$(TEST_CXX) $(TEST_LDFLAGS) -o $@ $^

$(TARGET_DIR)/send_command: send_command.c
	@gcc -std=c99 -Wall -Wextra -O3 $< -o $@

.PHONY: build flash start test format clean


include $(wildcard $(dep_dir)/*.d)
include $(wildcard $(test_dep_dir)/*.d)
