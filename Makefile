# Setting up compiler (and flags)
CC = gcc
CFLAGS = -Wall -O2 -Isrc
LDFLAGS = -ldl
SHARED_FLAGS = -shared -fPIC

# target directories
SRC_DIR = src
BIN_DIR = bin
MODULES_SRC_DIR = $(SRC_DIR)/modules
MODULES_DEST_DIR = $(BIN_DIR)/modules

SERVER_SRCS = $(SRC_DIR)/server.c
MODULE_NAMES = cpu disk mem

COMMON_SRC = $(SRC_DIR)/resource.c

MODULE_SO_FILES = $(patsubst %,$(MODULES_DEST_DIR)/%.so,$(MODULE_NAMES))

all: server modules

server: $(SERVER_SRCS) $(COMMON_SRC)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/server $(SERVER_SRCS) $(COMMON_SRC) $(LDFLAGS)
	@echo "Executable \"server\" created: $(BIN_DIR)/server"

modules: $(MODULE_SO_FILES)
	@echo "All modules are up to date."

$(MODULES_DEST_DIR)/%.so: $(MODULES_SRC_DIR)/%_module.c $(COMMON_SRC)
	@mkdir -p $(MODULES_DEST_DIR)
	$(CC) $(CFLAGS) $(SHARED_FLAGS) -o $@ $< $(COMMON_SRC)
	@echo "Module created: $@"

clean:
	@echo "Cleaning up build files."
	@rm -rf $(BIN_DIR)
	@echo "Done."

.PHONY: all modules clean
