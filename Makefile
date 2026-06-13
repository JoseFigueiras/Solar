BUILD_DIR := build

.PHONY: all configure run run-hardware clean

all: $(BUILD_DIR)/Makefile
	@$(MAKE) -C $(BUILD_DIR) --no-print-directory solar

configure: $(BUILD_DIR)/Makefile

$(BUILD_DIR)/Makefile: CMakeLists.txt
	@cmake -S . -B $(BUILD_DIR)

run: all
	@LIBGL_ALWAYS_SOFTWARE=1 $(BUILD_DIR)/solar

run-hardware: all
	@$(BUILD_DIR)/solar

clean:
	@cmake --build $(BUILD_DIR) --target clean
