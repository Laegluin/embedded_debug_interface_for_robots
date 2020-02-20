SERIAL ?= /dev/stm32_virtual_com_port

STM_CUBE_DIR := vendor/stm32_cube_f7_1.15.0

CC := arm-none-eabi-gcc
CXX := arm-none-eabi-g++
TEST_CXX := g++
SIZE := arm-none-eabi-size
OBJCOPY := arm-none-eabi-objcopy

INCLUDE_FLAGS := \
	-Isrc \
	-Isrc/config \
	-Iinclude \
	-I$(STM_CUBE_DIR)/Middlewares/Third_Party/FreeRTOS/Source/include \
	-I$(STM_CUBE_DIR)/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1 \
	-I$(STM_CUBE_DIR)/Middlewares/ST/STemWin/inc \
	-I$(STM_CUBE_DIR)/Drivers/BSP/STM32F7508-Discovery \
	-I$(STM_CUBE_DIR)/Drivers/STM32F7xx_HAL_Driver/Inc \
	-I$(STM_CUBE_DIR)/Drivers/CMSIS/Device/ST/STM32F7xx/Include \
	-I$(STM_CUBE_DIR)/Drivers/CMSIS/Core/Include

# TODO: enable opt and lto
OPT_FLAGS := -O0
ARCH_FLAGS := -mthumb -mcpu=cortex-m7 -mfpu=fpv5-sp-d16 -mfloat-abi=hard
CFLAGS := $(INCLUDE_FLAGS) $(ARCH_FLAGS) $(OPT_FLAGS) -std=c99 -g -Wall -Wextra
CXXFLAGS := $(INCLUDE_FLAGS) $(ARCH_FLAGS) $(OPT_FLAGS) -std=c++14 -fno-exceptions -g -Wall -Wextra
LDFLAGS := $(ARCH_FLAGS)
TEST_CXXFLAGS := $(INCLUDE_FLAGS) -std=c++14 -g -Wall -Wextra
TEST_LDFLAGS :=

TARGET_DIR := target
dep_dir := $(TARGET_DIR)/deps
object_dir := $(TARGET_DIR)/obj
test_dep_dir := $(dep_dir)/test
test_object_dir := $(object_dir)/test


objects := $(addprefix $(object_dir)/,$(addsuffix .o,$(basename $(notdir $(wildcard src/*.s src/*.cpp src/config/*.c src/config/*.cpp src/device/*.cpp src/ui/*.cpp)))))

test_objects := $(addprefix $(test_object_dir)/,$(addsuffix .o,$(basename $(notdir $(wildcard src/test/*.cpp)))))
test_objects += $(addprefix $(test_object_dir)/,$(filter-out main.o app.o interrupt_handlers.o,$(addsuffix .o,$(basename $(notdir $(wildcard src/*.cpp src/device/*.cpp)))))) 

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
	stm32f7xx_hal_tim.o \
	stm32f7xx_hal_tim_ex.o \
	stm32f7xx_hal_i2c.o \
	stm32f7508_discovery_ts.o \
	ft5336.o \
	list.o \
	queue.o \
	tasks.o \
	port.o \
	heap_4.o \
)

archives := $(STM_CUBE_DIR)/Middlewares/ST/STemWin/Lib/STemWin_CM7_wc32_ot_ARGB.a


build: $(TARGET_DIR)/firmware.bin

flash: $(TARGET_DIR)/firmware.bin $(TARGET_DIR)/send_command
	@$(abspath $(TARGET_DIR)/send_command) flash $(SERIAL) $<

start: $(TARGET_DIR)/send_command
	@$(abspath $(TARGET_DIR)/send_command) start $(SERIAL)

run: $(TARGET_DIR)/firmware.bin $(TARGET_DIR)/send_command
	@$(abspath $(TARGET_DIR)/send_command) flash $(SERIAL) $<
	@$(abspath $(TARGET_DIR)/send_command) start $(SERIAL)

test: $(TARGET_DIR)/test
	@$(abspath $<)

format:
	@clang-format -style=file -i src/*.cpp src/*.h src/test/*.cpp src/device/*.cpp src/device/*.h src/ui/*.cpp src/ui/*.h

clean:
	@rm -rf $(TARGET_DIR)

$(TARGET_DIR) $(object_dir) $(test_object_dir) $(dep_dir) $(test_dep_dir):
	@mkdir -p $@

# test objects
$(test_object_dir)/%.o: src/%.cpp | $(test_object_dir) $(test_dep_dir)
	@$(TEST_CXX) $(TEST_CXXFLAGS) -MT $@ -MD -MP -MF $(test_dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

$(test_object_dir)/%.o: src/device/%.cpp | $(test_object_dir) $(test_dep_dir)
	@$(TEST_CXX) $(TEST_CXXFLAGS) -MT $@ -MD -MP -MF $(test_dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

$(test_object_dir)/%.o: src/test/%.cpp | $(test_object_dir) $(test_dep_dir)
	@$(TEST_CXX) $(TEST_CXXFLAGS) -MT $@ -MD -MP -MF $(test_dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

# project objects
$(object_dir)/%.o: src/%.s | $(object_dir)
	@$(CXX) -g -c $< -o $@

$(object_dir)/%.o: src/%.cpp | $(object_dir) $(dep_dir)
	@$(CXX) $(CXXFLAGS) -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

$(object_dir)/%.o: src/device/%.cpp | $(object_dir) $(dep_dir)
	@$(CXX) $(CXXFLAGS) -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

$(object_dir)/%.o: src/ui/%.cpp | $(object_dir) $(dep_dir)
	@$(CXX) $(CXXFLAGS) -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

$(object_dir)/%.o: src/config/%.cpp | $(object_dir) $(dep_dir)
	@$(CXX) $(CXXFLAGS) -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

$(object_dir)/%.o: src/config/%.c | $(object_dir) $(dep_dir)
	@$(CC) $(CFLAGS) -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

# vendor objects
$(object_dir)/%.o: $(STM_CUBE_DIR)/Drivers/STM32F7xx_HAL_Driver/Src/%.c | $(object_dir) $(dep_dir)
	@$(CC) $(CFLAGS) -Wno-unused-parameter -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

$(object_dir)/%.o: $(STM_CUBE_DIR)/Drivers/BSP/STM32F7508-Discovery/%.c | $(object_dir) $(dep_dir)
	@$(CC) $(CFLAGS) -Wno-unused-parameter -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

$(object_dir)/%.o: $(STM_CUBE_DIR)/Drivers/BSP/Components/ft5336/%.c | $(object_dir) $(dep_dir)
	@$(CC) $(CFLAGS) -Wno-unused-parameter -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

$(object_dir)/%.o: $(STM_CUBE_DIR)/Middlewares/Third_Party/FreeRTOS/Source/%.c | $(object_dir) $(dep_dir)
	@$(CC) $(CFLAGS) -Wno-unused-parameter -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

$(object_dir)/%.o: $(STM_CUBE_DIR)/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/%.c | $(object_dir) $(dep_dir)
	@$(CC) $(CFLAGS) -Wno-unused-parameter -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

$(object_dir)/%.o: $(STM_CUBE_DIR)/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1/%.c | $(object_dir) $(dep_dir)
	@$(CC) $(CFLAGS) -Wno-unused-but-set-variable -Wno-unused-parameter -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

# actual binary for mcu
$(TARGET_DIR)/firmware.elf: $(objects) $(vendor_objects) $(archives) src/linker.ld | $(TARGET_DIR)
	@$(CXX) $(LDFLAGS) --specs=nosys.specs -Tsrc/linker.ld -Wl,--gc-sections -Wl,--wrap=_malloc_r -Wl,--wrap=_calloc_r -Wl,--wrap=_free_r -o $@ $(filter %.o %.a,$^)

$(TARGET_DIR)/firmware.bin: $(TARGET_DIR)/firmware.elf
	@$(SIZE) $<
	@$(OBJCOPY) -O binary $< $@

# test binary
$(TARGET_DIR)/test: $(test_objects) | $(TARGET_DIR)
	@$(TEST_CXX) $(TEST_LDFLAGS) -o $@ $^

$(TARGET_DIR)/send_command: send_command.c | $(TARGET_DIR)
	@gcc -std=c99 -Wall -Wextra -O3 $< -o $@

.PHONY: build flash start run test format clean


include $(wildcard $(dep_dir)/*.d)
include $(wildcard $(test_dep_dir)/*.d)
