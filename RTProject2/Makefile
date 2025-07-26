CC = gcc
CFLAGS = -Wall -Iinclude -O2
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

TARGETS = manager supply_chain chef baker seller customer gui
EXECUTABLES = $(addprefix $(BIN_DIR)/, $(TARGETS))

CONFIG_OBJ = $(OBJ_DIR)/config.o

all: $(EXECUTABLES)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(CONFIG_OBJ): $(SRC_DIR)/config.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/manager: $(SRC_DIR)/manager.c $(CONFIG_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/manager.c $(CONFIG_OBJ)

$(BIN_DIR)/supply_chain: $(SRC_DIR)/supply_chain.c $(CONFIG_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/supply_chain.c $(CONFIG_OBJ)

$(BIN_DIR)/chef: $(SRC_DIR)/chef.c $(CONFIG_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/chef.c $(CONFIG_OBJ)

$(BIN_DIR)/baker: $(SRC_DIR)/baker.c $(CONFIG_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/baker.c $(CONFIG_OBJ)

$(BIN_DIR)/seller: $(SRC_DIR)/seller.c $(CONFIG_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/seller.c $(CONFIG_OBJ)

$(BIN_DIR)/customer: $(SRC_DIR)/customer.c $(CONFIG_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/customer.c $(CONFIG_OBJ)

$(BIN_DIR)/gui: $(SRC_DIR)/gui.c $(CONFIG_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/gui.c $(CONFIG_OBJ) -lGL -lGLU -lglut -lm

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)
	-@ipcrm -M 0x1234 2>/dev/null || true
	-@ipcrm -Q 0x5678 2>/dev/null || true
	-@ipcrm -S 2341 2>/dev/null || true

run:
	./$(BIN_DIR)/manager

