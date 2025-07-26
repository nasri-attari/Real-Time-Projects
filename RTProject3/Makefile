CC = gcc
CFLAGS = -Wall -O2 -g
LDFLAGS = -lm -lpthread

GUI_LIBS = -lGL -lGLU -lglut

SRC_DIR = src
INCLUDE_DIR = include

MAIN_SRC    = $(SRC_DIR)/main.c
POLICE_SRC  = $(SRC_DIR)/police.c
GANG_SRC    = $(SRC_DIR)/gang.c
GUI_SRC     = $(SRC_DIR)/gui.c
CONFIG_SRC  = $(SRC_DIR)/config.c

MAIN_BIN    = main
POLICE_BIN  = police
GANG_BIN    = gang
GUI_BIN     = gui

COMMON_OBJS = $(CONFIG_SRC)

all: $(MAIN_BIN) $(POLICE_BIN) $(GANG_BIN) $(GUI_BIN)

$(MAIN_BIN): $(MAIN_SRC) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $^ -o $@ $(LDFLAGS)

$(POLICE_BIN): $(POLICE_SRC) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $^ -o $@ $(LDFLAGS)

$(GANG_BIN): $(GANG_SRC) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $^ -o $@ $(LDFLAGS)

$(GUI_BIN): $(GUI_SRC) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $^ -o $@ $(LDFLAGS) $(GUI_LIBS)

run: all
	./main
clean:
	rm -f $(MAIN_BIN) $(POLICE_BIN) $(GANG_BIN) $(GUI_BIN) *.o
	@ipcs -q | awk 'NR>3 {print $$2}' | xargs -r -n 1 ipcrm -q
	@ipcs -m | awk 'NR>3 {print $$2}' | xargs -r -n 1 ipcrm -m
	@ipcs -s | awk 'NR>3 {print $$2}' | xargs -r -n 1 ipcrm -s
