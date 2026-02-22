CC      = gcc
CFLAGS  = -Wall -Wextra -Iinclude
LDFLAGS =

SRC_DIR   = src
INC_DIR   = include
BUILD_DIR = build
BIN       = givit

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

install: $(BIN)
	sudo cp $(BIN) /usr/local/bin/

uninstall:
	sudo rm -f /usr/local/bin/$(BIN)

clean:
	rm -rf $(BUILD_DIR) $(BIN)

.PHONY: all install uninstall clean
