BUILD_DIR := build
CACHE_FILE := $(BUILD_DIR)/CMakeCache.txt

.PHONY: all configure run clean

all: configure
	@$(MAKE) -C $(BUILD_DIR) --no-print-directory solar

configure:
	@if [ -f "$(CACHE_FILE)" ] && ! grep -Fxq "CMAKE_HOME_DIRECTORY:INTERNAL=$$(pwd)" "$(CACHE_FILE)"; then \
		rm -rf "$(BUILD_DIR)"; \
	fi
	@cmake -S . -B $(BUILD_DIR)

run: all
	@LIBGL_ALWAYS_SOFTWARE=1 $(BUILD_DIR)/solar

clean:
	@cmake --build $(BUILD_DIR) --target clean
