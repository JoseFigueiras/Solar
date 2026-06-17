# AGENTS.md

## Overview
Solar system simulation/visualization written in C++ using raylib.

## Build & Run
This project uses CMake, wrapped by a Makefile for convenience.

- `make` — configure (if needed) and build the `solar` executable into `build/`.
- `make run` — build and run the project (uses software OpenGL via `LIBGL_ALWAYS_SOFTWARE=1`).
- `make clean` — clean build artifacts.
