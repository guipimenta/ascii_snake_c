# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A terminal-based snake animation game written in C — single source file, no external dependencies. ascii_lib.h contains all the logic for ascii drawing, including screen buffer handling, timer, keyboard input handling.

## Build & Run

```bash
# Compile
gcc -g animation.c -o animation

# Run
./animation
```

Controls: `KEY_UP`/`KEY_DOWN`/`KEY_LEFT`/`KEY_RIGHT` to change direction, `Q` to quit.

## Architecture

Everything lives in `animation.c`. Key components:

- **Terminal raw mode** — `enable_raw_mode()` / `disable_raw_mode()` toggle non-canonical, no-echo input via `termios`. `atexit` ensures cleanup on exit.
- **Screen buffer** — `screen_buffer` is a 100×50 `char**` used for off-screen rendering. `write_to_buffer()` stages characters; `render()` flushes to stdout with `\033[H\033[2J` to clear the screen.
- **Game loop** (in `main`) — reads a single keypress via `read_key()` (non-blocking), updates `SNAKE` velocity, calls `render()`, sleeps 1 second per frame.
- **Global state** — `orig_termios` (saved terminal settings), `FRAMES` (frame counter), `game_snake` (active snake).
