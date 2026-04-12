# Code Review: animation.c + ascii_lib.h

---

## Critical

### 1. Ctrl+C leaves terminal in raw mode permanently
**ascii_lib.h:19** — `enable_raw_mode()` registers `disable_raw_mode` via `atexit`. But `atexit` handlers are **not called when a process is terminated by a signal** (SIGINT, SIGTERM). Pressing Ctrl+C kills the process, skips all `atexit` handlers, and leaves the terminal in raw mode. The user's shell is broken until they manually run `reset` or `stty sane`.

Fix: install a signal handler for SIGINT/SIGTERM that calls `disable_raw_mode()` then re-raises the signal, or set `raw.c_lflag &= ~ISIG` to absorb Ctrl+C inside the game loop.

---

### 2. `read_key` mishandles non-blocking EAGAIN
**ascii_lib.h:36** — With `O_NONBLOCK` set, when no key is pressed `read()` returns **-1** with `errno == EAGAIN`, not 0. The check `if (read(...) == 0) return 0` only catches EOF. The function works by accident — `c` stays 0 from initialization — but this masks real I/O errors and makes the intent opaque. The same flaw repeats for the escape-sequence reads at lines 40 and 42.

Fix: check `read() <= 0` or explicitly test `errno == EAGAIN`.

---

### 3. Debug `printf` calls corrupt the game display every frame
**animation.c:65, 68, 91, 107** — These `printf` calls write directly to stdout while the terminal is in raw mode and the screen buffer is being rendered. They fire on every frame, printing garbage through the game output.

- L65: `printf("p1 (%d;%d)\n", ...)` — inside `check_collision`, called every frame
- L68: `printf("out of board, game over\n")`
- L91: `printf("All snake body part free\n")` — inside `free_snake_game`
- L107: `printf("score: %d\n", score)` — inside `draw_snake`, every frame

Remove these or gate them behind `#ifdef DEBUG`.

---

### 4. Score not reset between games
**animation.c:6** — `score` is a global initialized to 0 at program start but **never reset** inside `restart_game()`. After a game-over and restart, the score accumulates from the previous game.

---

### 5. `scanf` reads leftover newline, skips restart prompt
**animation.c:250** — After `restart_game()` returns, `scanf("%c", &restart)` reads the `\n` left in stdin from the previous input event. The user never gets a chance to type 'y'; the program sees '\n', treats it as not-'y', and exits immediately.

Fix: `scanf(" %c", &restart)` — the leading space skips whitespace including newlines.

---

## Warning

### 6. `malloc` return values never checked
**animation.c:109, 114, 152, 176, 187** — None of the `malloc` calls check for NULL. On allocation failure the code immediately dereferences the returned pointer, causing a crash with no diagnostic.

---

### 7. `print_snake_coordinates` ignores its parameter
**animation.c:95** — The signature is `print_snake_coordinates(SNAKE snake)` but the body iterates `game_snake.head` (the global) instead of `snake.head`. The parameter is silently dead.

---

### 8. `enable_raw_mode` registers `atexit` on every game restart
**ascii_lib.h:19** — `atexit(disable_raw_mode)` is called inside `enable_raw_mode()`. Each call to `restart_game()` calls `enable_raw_mode()` again, pushing another entry onto the atexit stack. After N restarts, `disable_raw_mode()` runs N+1 times on exit. The calls are harmless today (tcsetattr is idempotent), but it also means `orig_termios` is overwritten on each restart — if the terminal is somehow still in raw mode when enable is called again, the "saved" state is raw mode and restoring it won't fix anything.

Fix: register the atexit handler once in `main`, not inside `enable_raw_mode`.

---

### 9. Missing `ISIG` flag — Ctrl+C is not handled in-game
**ascii_lib.h:22** — Raw mode only clears `ECHO | ICANON`. It does not clear `ISIG`, so Ctrl+C still sends SIGINT. Combined with issue #1 (no signal handler), this is the most common way users will break their terminal.

---

### 10. `rand()` not seeded — food always spawns at identical positions
**animation.c:4** — `rand_number()` uses `rand()` with no `srand()` call. Every run of the program produces the exact same food sequence.

Fix: `srand((unsigned)time(NULL))` once in `main`. Add `#include <time.h>`.

---

### 11. Uninitialized fields in `snake_body_part` allocations
**animation.c:152–156, 187–191** — `add_snake_body_part()` sets only `nodeid`, `x`, `y`, `next`; `vx`, `vy`, and `direction` are left uninitialized. The same applies to body segments created in `restart_game()`. Reading them is UB; passing the struct by value copies garbage.

---

### 12. `render` clears screen without moving cursor to home
**ascii_lib.h:147** — `printf("\033[2J")` erases the display but does not reposition the cursor. Output on the next frame starts from wherever the cursor happened to be, causing the display to drift downward. Should be `printf("\033[H\033[2J")`. (CLAUDE.md already documents this as the intended sequence — this is a regression.)

---

### 13. Food can spawn on the snake body
**animation.c:50–52** — `populate_snake_food` picks random coordinates without checking whether any snake segment already occupies that cell. The food character is overwritten by `#` and then immediately eaten on the next head overlap.

---

## Suggestion

### 14. Magic numbers without named constants
- `100`, `50` — buffer dimensions (animation.c:169–170)
- `3` — initial snake length (animation.c:182)
- `100000` — frame delay in microseconds (animation.c:237)

Define these with `#define` at the top of the file.

---

### 15. Unused variables and dead struct fields
- `int temp` — declared at animation.c:108, never used.
- `int i` in `draw_snake` — incremented at line 143 but never read.
- `snake_body_part.direction` — never written or read anywhere.
- `long FRAMES` — declared at animation.c:5, never incremented.

---

### 16. Duplicate `draw_snake` declaration
`draw_snake` is declared at both **animation.c:34** and **animation.c:43** (after the `#include`). The second is redundant.

---

### 17. `add_snake_body_part()` should use `(void)`
**animation.c:45** — In C, `void f()` means "unspecified parameters", not "no parameters". Use `void add_snake_body_part(void)` for the declaration and definition.

---

### 18. Frame timing drifts under load
**animation.c:237** — `usleep(100000)` sleeps exactly 100 ms, ignoring the time spent in `render`, `draw_snake`, and `read_key`. The effective frame rate is slightly below 10 fps and worsens as the snake grows. A `clock_gettime`-based delta approach eliminates drift.

---

### 19. `rand_number` formula is semantically misleading
**animation.c:4** — `(rand() % max) + min` treats `max` as a span, not an upper bound. `rand_number(1, 98)` returns [1, 98], not [1, 98] inclusive of 98. For the current call sites this is accidentally correct, but the parameter named `max` implies inclusive upper bound, which it is not.

Fix for true `[min, max]` range: `(rand() % (max - min + 1)) + min`.
