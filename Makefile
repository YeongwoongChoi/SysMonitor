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

OBJ_DIR = $(BIN_DIR)/obj
LIB_DIR = $(BIN_DIR)/lib

SERVER_SRCS = $(SRC_DIR)/server.c
MODULE_NAMES = cpu disk mem

COMMON_SRC = $(SRC_DIR)/resource.c
COMMON_OBJ = $(OBJ_DIR)/resource.o
STATIC_LIB = $(LIB_DIR)/libresource.a

MODULE_SO_FILES = $(patsubst %,$(MODULES_DEST_DIR)/%.so,$(MODULE_NAMES))

all: server modules

server: $(SERVER_SRCS) $(STATIC_LIB)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/server $(SERVER_SRCS) -L$(LIB_DIR) -lresource $(LDFLAGS)
	@echo "Executable \"server\" created: $(BIN_DIR)/server"

modules: $(MODULE_SO_FILES)
	@echo "All modules are up to date."

$(STATIC_LIB): $(COMMON_OBJ)
	@mkdir -p $(LIB_DIR)
	@echo "Creating static library: $@"
	ar rcs $@ $^

$(COMMON_OBJ): $(COMMON_SRC)
	@mkdir -p $(OBJ_DIR)
	@echo "Compiling common source: $<"
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

$(MODULES_DEST_DIR)/%.so: $(MODULES_SRC_DIR)/%_module.c $(STATIC_LIB)
	@mkdir -p $(MODULES_DEST_DIR)
	$(CC) $(CFLAGS) $(SHARED_FLAGS) -o $@ $< -L$(LIB_DIR) -lresource
	@echo "Module created: $@"

clean:
	@echo "Cleaning up build files."
	@rm -rf $(BIN_DIR)
	@echo "Done."

.PHONY: all modules clean
