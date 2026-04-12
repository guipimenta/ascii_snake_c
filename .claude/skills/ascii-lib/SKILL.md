---
name: ascii-lib
description: Use this skill when the user asks about ascii_lib.h, wants to build a new game or animation with it, or asks how to use the screen buffer, input handling, or rendering functions in this project.
---

# ascii_lib.h — ASCII Game Engine Reference

Single-header C library (`ascii_lib.h`). Include it in any `.c` file. No external dependencies — only `<stdio.h>`, `<stdlib.h>`, and optionally `<termios.h>` / `<fcntl.h>` / `<unistd.h>`.

---

## Feature flags (define before including)

| Macro | Effect |
|---|---|
| `#define ASCII_LIB_KEY_HANDLER` | Enables raw-mode terminal input and `read_key()`. Requires POSIX. |
| `#define GAME_RENDERING_FN <code>` | Injects custom rendering code into the `render()` call. The code runs after `clean_buffer()` and `draw_map()`, before flushing to stdout. |
| `#define DEBUG` | Enables `DBG()` macro and in-frame debug overlay (right-aligned, top rows). |

---

## Data structures

```c
typedef struct screen_buffer {
    int width;
    int height;
    char **buffer;   // 2-D array: buffer[y][x]
} screen_buffer;
```

---

## Screen buffer API

### `screen_buffer init_buffer(screen_buffer buffer)`
Allocates `buffer.buffer` for the given `width` × `height`. Caller must set `width` and `height` before calling. Fills all cells with `' '`. Returns the populated struct (pass by value, receive by value).

```c
screen_buffer sb = { .width = 80, .height = 24 };
sb = init_buffer(sb);
```

### `screen_buffer init_buffer_from_terminal()`
Reads the current terminal size via `ioctl(TIOCGWINSZ)` and initializes a buffer to match it. Sets `height = rows - 1` (reserves one line to avoid scroll jitter). Requires `<sys/ioctl.h>` and `<unistd.h>` (included automatically).

```c
screen_buffer sb = init_buffer_from_terminal(); // fits current window
```

### `void write_to_buffer(screen_buffer buffer, int x, int y, char val)`
Writes `val` at column `x`, row `y`. Silently clamps: does nothing if `x < 0`, `y < 0`, `x >= width`, or `y >= height`.

### `void clean_buffer(screen_buffer buffer)`
Resets every cell to `' '`. Called automatically by `render()` each frame.

### `void free_buffer(screen_buffer buffer)`
Frees all rows then the row-pointer array. Call on shutdown.

---

## Debug overlay (requires `#define DEBUG`)

```c
DBG(fmt, ...)   // printf-style — buffers a message (no-op without DEBUG)
```

- Up to `DEBUG_MAX_LINES` (10) messages per frame, each up to `DEBUG_LINE_LEN` (48) chars.
- Messages are written **right-aligned** to the top rows of the screen buffer by `_flush_debug()`, which is called automatically inside `render()`.
- The buffer is cleared every frame — call `DBG()` each frame to keep a message visible.
- Without `DEBUG`, `DBG()` expands to `((void)0)` — zero overhead in release builds.
- Instrumented automatically: `read_key()` logs key name; `write_to_buffer()` logs OOB writes; `render()` logs the `dt` value; `init_buffer_from_terminal()` logs detected size.

```c
// Example: show snake head position each frame
DBG("head (%d, %d)", snake.x, snake.y);
```

---

## Rendering API

### `void draw_map(screen_buffer buffer)`
Draws a border: `=` on top and bottom rows, `|` on left and right columns.

### `int render(int dt, screen_buffer buffer)`
Full-frame render pipeline:
1. Moves cursor to home and clears the terminal (`\033[H\033[2J`).
2. Calls `clean_buffer()`.
3. Calls `draw_map()`.
4. Executes `GAME_RENDERING_FN` (if defined) — this is where game objects are drawn.
5. Calls `_flush_debug()` — renders any pending `DBG()` messages right-aligned at the top; no-op without `DEBUG`.
6. Flushes `buffer` to stdout row by row.

`dt` is passed in but currently unused — reserved for delta-time logic. Returns `0` (or whatever `GAME_RENDERING_FN` sets via `res`).

---

## Input API (requires `ASCII_LIB_KEY_HANDLER`)

### `void enable_raw_mode()`
Disables canonical mode and echo (`ICANON | ECHO`). Sets stdin to non-blocking (`O_NONBLOCK`). Registers `disable_raw_mode` via `atexit` for automatic cleanup.

### `void disable_raw_mode()`
Restores original `termios` settings and clears `O_NONBLOCK`. Safe to call multiple times (idempotent via `atexit`).

### `int read_key()`
Non-blocking single-keypress read. Returns:
- `0` — no key pressed
- `KEY_UP` (1000) — arrow up (`\033[A`)
- `KEY_DOWN` (1001) — arrow down (`\033[B`)
- `KEY_RIGHT` (1002) — arrow right (`\033[C`)
- `KEY_LEFT` (1003) — arrow left (`\033[D`)
- Any other `char` — its ASCII value (e.g. `'q'`, `'w'`)

---

## Minimal game skeleton

```c
#define DEBUG                         // remove for release
#define ASCII_LIB_KEY_HANDLER
#define GAME_RENDERING_FN  draw_my_objects(sb);
#include "ascii_lib.h"
#include <unistd.h>

screen_buffer sb;

void draw_my_objects(screen_buffer buf) {
    write_to_buffer(buf, 5, 5, 'X');
    DBG("obj at (5,5)");
}

int main() {
    sb = init_buffer_from_terminal(); // auto-size to window
    enable_raw_mode();

    while (1) {
        int key = read_key();
        if (key == 'q') break;
        render(0, sb);
        usleep(100000); // ~10 fps
    }

    free_buffer(sb);
    return 0;
}
```

---

## Common pitfalls

- `buffer[y][x]` — y (row) is the first index, x (column) is second. Pass `x` first to `write_to_buffer`, but remember the internal layout.
- `GAME_RENDERING_FN` is token-pasted as raw C code inside `render()`. It has access to local variable `res` (int) and parameter `buffer` (screen_buffer).
- `render()` calls `clean_buffer()` every frame — do not write to the buffer before `render()` expecting it to persist.
- `DBG()` messages are also cleared every frame by `_flush_debug()`. Re-emit them each frame if you want them to stay visible.
- Debug overlay occupies the top rows — if `_debug_line_count` reaches 10, additional `DBG()` calls are silently dropped that frame.
- `enable_raw_mode()` must be called before `read_key()`. Without it, reads block on a full newline.
- No built-in frame timer: the caller is responsible for `sleep`/`usleep`/`nanosleep`.
