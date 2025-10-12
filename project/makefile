# temporarily generated makefile for testing

# Builds and runs the test_input program using SDL2

# Compiler and flags
CC = gcc
CFLAGS = -std=c11 -g -Wall -Werror
INCLUDE = ./include
SRC = ./src/geometry.c ./src/draw_input.c ./src/raster.c ./src/fourier.c

# SDL2 configuration (uses sdl2-config to find includes and libs)
SDL_CFLAGS  = $(shell sdl2-config --cflags)
SDL_LDFLAGS = $(shell sdl2-config --libs)

# Output directory
BIN = ./bin

# Default target: build SDL input test
all: $(BIN)/main

# --- Build rules ---

# Ensure bin directory exists
$(BIN):
	mkdir -p $(BIN)

# Build the test_input executable
$(BIN)/main: $(SRC) ./main.c | $(BIN)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -I$(INCLUDE) $^ -o $@ $(SDL_LDFLAGS) -lm

# --- Utility targets ---

run:
	@echo "Running main..."
	@$(BIN)/main

clean:
	rm -rf $(BIN)/*
	rm -f .DS_Store
	rm -f ../.DS_Store
