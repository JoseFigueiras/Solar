BUILD_DIR := build

.PHONY: all configure run clean

all: $(BUILD_DIR)/Makefile
	@$(MAKE) -C $(BUILD_DIR) --no-print-directory solar

configure: $(BUILD_DIR)/Makefile

$(BUILD_DIR)/Makefile: CMakeLists.txt
	@cmake -S . -B $(BUILD_DIR)

run: all
	@$(BUILD_DIR)/solar

clean:
	@cmake --build $(BUILD_DIR) --target clean
