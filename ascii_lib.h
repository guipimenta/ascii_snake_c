#ifndef ASCII_LIB
#define ASCII_LIB

#ifdef ASCII_LIB_KEY_HANDLER
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

struct termios orig_termios;

void disable_raw_mode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); }

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
      if (seq[1] == 'A')
        return KEY_UP;
      if (seq[1] == 'B')
        return KEY_DOWN;
      if (seq[1] == 'C')
        return KEY_RIGHT;
      if (seq[1] == 'D')
        return KEY_LEFT;
    }
    return '\033';
  }
  return c;
}

#endif

#ifdef DEBUG

#endif

#include <stdio.h>
#include <stdlib.h>
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
void write_to_buffer(screen_buffer buffer, int x, int y, char val);
void clean_buffer(screen_buffer buffer);
void free_buffer(screen_buffer buffer);

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

void write_to_buffer(screen_buffer buffer, int x, int y, char val) {
  if (x >= buffer.width || y >= buffer.height) {
    return;
  }
  if (x < 0 || y < 0) {
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
  printf("\033[2J");

  clean_buffer(buffer);

  draw_map(buffer);
  int res = 0;
#ifdef GAME_RENDERING_FN
  GAME_RENDERING_FN
#endif

  for (int i = 0; i < buffer.height; i++) {
    for (int j = 0; j < buffer.width; j++) {
      printf("%c", buffer.buffer[i][j]);
    }
    printf("\n");
  }
  printf("\n");

  return res;
}

#endif
