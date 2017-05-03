GCC ?= gcc
SRC_DIR = src
BIN_DIR ?= bin
TARGET = $(BIN_DIR)/test_app
SOURCES = $(wildcard $(SRC_DIR)/*.c)

CFLAGS += -std=gnu99 -O3 -Wall -Wextra

all: $(BIN_DIR) $(TARGET)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(TARGET): $(SOURCES)
	$(GCC) $(CFLAGS) $^ -o $@

.PHONY: test
test: all
	@./run_all.sh

.PHONY: clean
clean:
	@rm -rf $(BIN_DIR)
