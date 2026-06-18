BUILD_DIR := build
CACHE_FILE := $(BUILD_DIR)/CMakeCache.txt
CMAKE_BUILD_TYPE ?= RelWithDebInfo

# Allow `make run <count>` / `make profile <count>` to forward the asteroid
# count to the program. Any goals after `run`/`profile` are captured here and
# turned into no-op targets below so Make does not try to build them as files.
ifneq (,$(filter run profile,$(firstword $(MAKECMDGOALS))))
RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
$(eval $(RUN_ARGS):;@:)
endif

.PHONY: all configure run profile clean

all: configure
	@$(MAKE) -C $(BUILD_DIR) --no-print-directory solar

configure:
	@if [ -f "$(CACHE_FILE)" ] && ! grep -Fxq "CMAKE_HOME_DIRECTORY:INTERNAL=$$(pwd)" "$(CACHE_FILE)"; then \
		rm -rf "$(BUILD_DIR)"; \
	fi
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE)

run: all
	@LIBGL_ALWAYS_SOFTWARE=1 $(BUILD_DIR)/solar $(RUN_ARGS)

profile: all
	@LIBGL_ALWAYS_SOFTWARE=1 valgrind --tool=callgrind --cache-sim=yes --branch-sim=yes $(BUILD_DIR)/solar $(RUN_ARGS)

clean:
	@cmake --build $(BUILD_DIR) --target clean
