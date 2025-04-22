# Compiler and flags
CC = gcc
CFLAGS = -Wall -Iinclude -w

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Source and object files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))

# Targets for executables
MAIN_TARGET = $(BIN_DIR)/simulation
REFEREE_TARGET = $(BIN_DIR)/referee
PLAYER_TARGET = $(BIN_DIR)/player
GUI_TARGET = $(BIN_DIR)/gui

# Default target
all: $(MAIN_TARGET) $(REFEREE_TARGET) $(PLAYER_TARGET) $(GUI_TARGET)

# Build the main referee executable
$(MAIN_TARGET): $(OBJ_DIR)/main.o $(OBJ_DIR)/config.o | $(BIN_DIR)
	$(CC) $^ -o $(MAIN_TARGET) -lGL -lGLU -lglut -lm

$(REFEREE_TARGET): $(OBJ_DIR)/referee.o $(OBJ_DIR)/config.o | $(BIN_DIR)
	$(CC) $^ -o $(REFEREE_TARGET) -lGL -lGLU -lglut -lm

$(PLAYER_TARGET): $(OBJ_DIR)/player.o $(OBJ_DIR)/config.o | $(BIN_DIR)
	$(CC) $^ -o $(PLAYER_TARGET) -lGL -lGLU -lglut -lm

$(GUI_TARGET): $(OBJ_DIR)/gui_main.o $(OBJ_DIR)/gui.o $(OBJ_DIR)/config.o | $(BIN_DIR)
	$(CC) $^ -o $(GUI_TARGET) -lGL -lGLU -lglut -lm


# Compile each source file to an object file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create directories if they don't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Run the simulation (main process)
run: $(MAIN_TARGET)
	$(MAIN_TARGET)

# Remove old FIFOs (added target)
clean-fifos:
	rm -f /tmp/rope_pulling_*.fifo

# Clean everything
clean: clean-fifos
	rm -rf $(OBJ_DIR) $(BIN_DIR)
