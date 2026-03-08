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

/*
 * Data Structures Definition
 * */

typedef struct screen_buffer {
  int width;
  int height;
  char **buffer;
} screen_buffer;

/*
 * functions declaration
 */

screen_buffer init_buffer(screen_buffer buffer);
void write_to_buffer(screen_buffer buffer, int x, int y, char val);
void clean_buffer(screen_buffer buffer);
void free_buffer(screen_buffer buffer);
screen_buffer init_buffer(screen_buffer buffer);
#endif
