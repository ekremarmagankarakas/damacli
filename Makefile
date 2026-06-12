BUILD_DIR := build
BIN       := $(BUILD_DIR)/damacli
SOURCES   := $(shell find src tests -name '*.cc' -o -name '*.h' 2>/dev/null)

.PHONY: all configure build run test clean format check install-hooks help

all: build

configure:
	cmake -S . -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR)

run: build
	./$(BIN)

test: build
	cd $(BUILD_DIR) && ctest --output-on-failure

clean:
	rm -rf $(BUILD_DIR)

format:
	clang-format -i $(SOURCES)

check:
	clang-format --dry-run --Werror $(SOURCES)

install-hooks:
	git config core.hooksPath .githooks
	@echo "pre-commit hook installed (.githooks/pre-commit)"

help:
	@echo "Targets:"
	@echo "  build          Configure + compile"
	@echo "  run            Build + run binary"
	@echo "  test           Build + run unit tests (ctest)"
	@echo "  clean          Remove build dir"
	@echo "  format         Format all sources in place"
	@echo "  check          Verify formatting without writing"
	@echo "  install-hooks  Wire .githooks/ into this git repo"
