export GCC ?= gcc
SRC_DIR = src
BIN_DIR ?= bin
TARGET = $(BIN_DIR)/taskset
SOURCES = $(wildcard $(SRC_DIR)/*.c)
PREFIX ?= /usr

CFLAGS  += -std=gnu99 -O3 -Wall -Wextra
LDFLAGS += -static

all: $(BIN_DIR) $(TARGET)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(TARGET): $(SOURCES)
	$(GCC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

.PHONY: install
install: all
	install -v -m 755 $(TARGET) $(PREFIX)/bin/

.PHONY: test
test: all
	@$(MAKE) -C test/ test PATH="${PWD}/bin":${PATH}

.PHONY: clean
clean:
	@rm -rf $(BIN_DIR)
	@$(MAKE) -C test/ clean
