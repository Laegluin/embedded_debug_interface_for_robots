COMMON_CXXFLAGS := -std=c++14 -Wall -Wextra -Iinclude
CXX := arm-none-eabi-g++
CXXFLAGS := $(COMMON_CXXFLAGS) -O3 -flto
TEST_CXX := g++
TEST_CXXFLAGS := $(COMMON_CXXFLAGS) -DTEST

TARGET_DIR := target
dep_dir := $(TARGET_DIR)/deps
object_dir := $(TARGET_DIR)/obj
test_dep_dir := $(dep_dir)/test
test_object_dir := $(object_dir)/test

objects := $(addprefix $(object_dir)/,main.o)
test_objects := $(addprefix $(test_object_dir)/,test.o)


build: $(TARGET_DIR)/firmware.elf

test: $(TARGET_DIR)/test
	$(abspath $<)

format:
	clang-format -style=file -i src/*.cpp

clean:
	rm -rf $(TARGET_DIR)

$(TARGET_DIR) $(object_dir) $(test_object_dir) $(dep_dir) $(test_dep_dir):
	mkdir -p $@

$(object_dir)/%.o: src/%.cpp | $(object_dir) $(dep_dir)
	$(CXX) $(CXXFLAGS) -MT $@ -MD -MP -MF $(dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

$(test_object_dir)/%.o: src/%.cpp | $(test_object_dir) $(test_dep_dir)
	$(TEST_CXX) $(TEST_CXXFLAGS) -MT $@ -MD -MP -MF $(test_dep_dir)/$(basename $(notdir $@)).d -c $< -o $@

# TODO: configure linker
# actual binary for mcu
$(TARGET_DIR)/firmware.elf: $(objects) | $(TARGET_DIR)
	$(CXX) $(CXXFLAGS) --specs=nosys.specs -o $@ $^

# test binary
$(TARGET_DIR)/test: $(test_objects) | $(TARGET_DIR)
	$(TEST_CXX) $(TEST_CXXFLAGS) -o $@ $^

.PHONY: build test format clean


include $(wildcard $(dep_dir)/*.d)
include $(wildcard $(test_dep_dir)/*.d)
