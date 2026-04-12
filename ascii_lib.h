#ifndef ASCII_LIB
#define ASCII_LIB

#ifdef DEBUG
#include <stdio.h>
#include <string.h>
#define DEBUG_MAX_LINES 10
#define DEBUG_LINE_LEN  48
static char _debug_lines[DEBUG_MAX_LINES][DEBUG_LINE_LEN];
static int  _debug_line_count = 0;

#define DBG(fmt, ...) do { \
  if (_debug_line_count < DEBUG_MAX_LINES) { \
    snprintf(_debug_lines[_debug_line_count++], DEBUG_LINE_LEN, fmt, ##__VA_ARGS__); \
  } \
} while (0)
#else
#define DBG(fmt, ...) ((void)0)
#endif

#ifdef ASCII_LIB_KEY_HANDLER
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

struct termios orig_termios;

void disable_raw_mode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
}

void enable_raw_mode() {
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disable_raw_mode);

  struct termios raw = orig_termios;
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

#define KEY_UP 1000
#define KEY_DOWN 1001
#define KEY_RIGHT 1002
#define KEY_LEFT 1003

int read_key() {
  char c = 0;
  if (read(STDIN_FILENO, &c, 1) == 0)
    return 0;
  if (c == '\033') {
    char seq[2];
    if (read(STDIN_FILENO, &seq[0], 1) == 0)
      return '\033';
    if (read(STDIN_FILENO, &seq[1], 1) == 0)
      return '\033';
    if (seq[0] == '[') {
      if (seq[1] == 'A') { DBG("key=UP"); return KEY_UP; }
      if (seq[1] == 'B') { DBG("key=DOWN"); return KEY_DOWN; }
      if (seq[1] == 'C') { DBG("key=RIGHT"); return KEY_RIGHT; }
      if (seq[1] == 'D') { DBG("key=LEFT"); return KEY_LEFT; }
    }
    return '\033';
  }
  return c;
}

#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
/*
 * Data Structures Definition
 * */

typedef struct screen_buffer {
  int width;
  int height;
  char **buffer;
} screen_buffer;

/*
 * functions declaration - low level declarions
 */

screen_buffer init_buffer(screen_buffer buffer);
screen_buffer init_buffer_from_terminal();
void write_to_buffer(screen_buffer buffer, int x, int y, char val);
void clean_buffer(screen_buffer buffer);
void free_buffer(screen_buffer buffer);

#ifdef DEBUG
static void _flush_debug(screen_buffer buffer) {
  for (int l = 0; l < _debug_line_count; l++) {
    int len = (int)strlen(_debug_lines[l]);
    int x   = buffer.width - len - 1;
    for (int c = 0; c < len; c++)
      write_to_buffer(buffer, x + c, l, _debug_lines[l][c]);
  }
  _debug_line_count = 0;
}
#else
static inline void _flush_debug(screen_buffer buffer) { (void)buffer; }
#endif

/*
 * Function implementation
 **/

screen_buffer init_buffer(screen_buffer buffer) {
  buffer.buffer = (char **)malloc(buffer.height * sizeof(char *));
  for (int j = 0; j < buffer.height; j++) {
    buffer.buffer[j] = (char *)malloc(sizeof(char) * buffer.width);
  }

  clean_buffer(buffer);
  return buffer;
}

screen_buffer init_buffer_from_terminal() {
  struct winsize ws;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  screen_buffer buffer;
  buffer.width  = ws.ws_col;
  buffer.height = ws.ws_row - 1;
  DBG("terminal %dx%d", buffer.width, buffer.height);
  return init_buffer(buffer);
}

void write_to_buffer(screen_buffer buffer, int x, int y, char val) {
  if (x >= buffer.width || y >= buffer.height) {
    DBG("OOB write (%d;%d) val=%c", x, y, val);
    return;
  }
  if (x < 0 || y < 0) {
    DBG("OOB write (%d;%d) val=%c", x, y, val);
    return;
  }
  buffer.buffer[y][x] = val;
}

void clean_buffer(screen_buffer buffer) {
  for (int i = 0; i < buffer.height; i++) {
    for (int j = 0; j < buffer.width; j++) {
      buffer.buffer[i][j] = ' ';
    }
  }
}

void free_buffer(screen_buffer buffer) {
  for (int i = 0; i < buffer.height; i++) {
    free(buffer.buffer[i]);
  }
  free(buffer.buffer);
}

/*
 * game-related functions
 */

void draw_map(screen_buffer buffer);
int render(int dt, screen_buffer buffer);

void draw_map(screen_buffer buffer) {
  for (int i = 0; i < buffer.height; i++) {
    for (int j = 0; j < buffer.width; j++) {
      if (i == 0 || i == buffer.height - 1) {
        write_to_buffer(buffer, j, i, '=');
      } else {
        if (j == 0 || j == buffer.width - 1) {
          write_to_buffer(buffer, j, i, '|');
        }
      }
    }
  }
}

int render(int dt, screen_buffer buffer) {
  DBG("frame=%d", dt);
  printf("\033[H\033[2J");

  clean_buffer(buffer);

  draw_map(buffer);
  int res = 0;
#ifdef GAME_RENDERING_FN
  GAME_RENDERING_FN
#endif

  _flush_debug(buffer);

  for (int i = 0; i < buffer.height; i++) {
    for (int j = 0; j < buffer.width; j++) {
      printf("%c", buffer.buffer[i][j]);
    }
    printf("\n");
  }

  return res;
}

#endif
