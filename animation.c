#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
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

int rand_number(int min, int max) { return (rand() % max) + min; }

long FRAMES = 0;
int score = 0;

typedef struct screen_buffer {
  int width;
  int height;
  char **buffer;
} screen_buffer;

typedef struct snake_body_part {
  int x;
  int y;
  int vx;
  int vy;
  int direction;
  int nodeid;
  struct snake_body_part *next;
} snake_body_part;

typedef struct snake {
  int length;
  snake_body_part *head;
  snake_body_part *tail;
} SNAKE;

typedef struct snake_food {
  int x;
  int y;
  int active;
} SNAKE_FOOD;

SNAKE_FOOD snake_food;
SNAKE game_snake;

screen_buffer init_buffer(screen_buffer buffer);

void write_to_buffer(screen_buffer buffer, int x, int y, char val);

int draw_snake(screen_buffer buffer, SNAKE snake, int dt);

void clean_buffer(screen_buffer buffer);

void add_snake_body_part();

void populate_snake_food(screen_buffer buffer) {
  if (!snake_food.active) {
    // redraw
    snake_food.x = rand_number(1, buffer.width - 2);
    snake_food.y = rand_number(1, buffer.height - 2);
    snake_food.active = 1;
  }

  write_to_buffer(buffer, snake_food.x, snake_food.y, '*');
}

int check_collision(screen_buffer buffer) {
  if (snake_food.x == game_snake.head->x &&
      snake_food.y == game_snake.head->y) {
    snake_food.active = 0;
    score++;
    add_snake_body_part();
  }
  printf("p1 (%d;%d)\n", game_snake.head->x, game_snake.head->y);
  if (game_snake.head->y <= 0 || game_snake.head->y >= buffer.height - 1 ||
      game_snake.head->x <= 0 || game_snake.head->x >= buffer.width - 1) {
    printf("out of board, game over\n");
    return -1;
  }

  snake_body_part *p1 = game_snake.head;
  snake_body_part *p2 = p1->next;
  while (p2 != NULL) {
    if (p1->x == p2->x && p1->y == p2->y) {
      return -1;
    }
    p2 = p2->next;
  }

  return 0;
}

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
  int res = draw_snake(buffer, game_snake, dt);
  populate_snake_food(buffer);

  for (int i = 0; i < buffer.height; i++) {
    for (int j = 0; j < buffer.width; j++) {
      printf("%c", buffer.buffer[i][j]);
    }
    printf("\n");
  }
  printf("\n");

  return res;
}

void clean_buffer(screen_buffer buffer) {
  for (int i = 0; i < buffer.height; i++) {
    for (int j = 0; j < buffer.width; j++) {
      buffer.buffer[i][j] = ' ';
    }
  }
}

screen_buffer init_buffer(screen_buffer buffer) {
  buffer.buffer = (char **)malloc(buffer.height * sizeof(char *));
  for (int j = 0; j < buffer.height; j++) {
    buffer.buffer[j] = (char *)malloc(sizeof(char) * buffer.width);
  }

  clean_buffer(buffer);
  return buffer;
}

void free_buffer(screen_buffer buffer) {
  for (int i = 0; i < buffer.height; i++) {
    free(buffer.buffer[i]);
  }
  free(buffer.buffer);
}

void free_snake_game(SNAKE snake) {
  snake_body_part *part = snake.head;
  while (part != NULL) {
    snake_body_part *next = part->next;
    free(part);
    part = next;
  }
  printf("All snake body part free\n");
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

void print_snake_coordinates(SNAKE snake) {
  snake_body_part *part = game_snake.head;
  int i = 0;
  while (part != NULL) {
    printf("(%d;%d;%d)\n", part->nodeid, part->x, part->y);
    part = part->next;
    i++;
  }
  printf("number of snake parts: %d\n", i);
}

int draw_snake(screen_buffer buffer, SNAKE snake, int dt) {
  snake_body_part *part = snake.head;
  int i = 0;
  // printf("coordinates before move: \n");
  // print_snake_coordinates(snake);
  printf("score: %d\n", score);
  int temp;
  int **node_coordinate = (int **)malloc(sizeof(int *) * snake.length);
  // printf("allocated elments of n. %d\n", snake.length);
  for (int j = 0; j < snake.length; j++) {
    if (part == NULL) {
      break;
    }
    node_coordinate[j] = (int *)malloc(sizeof(int) * 2);
    node_coordinate[j][0] = part->x;
    node_coordinate[j][1] = part->y;
    part = part->next;
  }

  // printf("stored coordinates\n");
  // for (int j = 0; j < snake.length; j++)
  // printf("(%d;%d;%d) ", j, node_coordinate[j][0], node_coordinate[j][1]);
  // printf("\n");
  part = snake.head->next;
  for (int j = 0; j < snake.length; j++) {
    if (part == NULL) {
      free(node_coordinate[j]);
      break;
    }
    part->x = node_coordinate[j][0];
    part->y = node_coordinate[j][1];
    part = part->next;
    free(node_coordinate[j]);
  }

  // printf("free memory\n");

  // printf("free element n. %d\n", snake.length);
  free(node_coordinate);

  // printf("done moving coordinates\n");
  snake.head->x = snake.head->x + snake.head->vx;
  snake.head->y = snake.head->y + snake.head->vy;
  int val = check_collision(buffer);

  // printf("coordnates after move\n");
  // print_snake_coordinates(snake);
  part = snake.head;

  while (part != NULL) {
    write_to_buffer(buffer, part->x, part->y, '#');
    part = part->next;
    i++;
  }
  return val;
}

void add_snake_body_part() {
  game_snake.length++;
  snake_body_part *tail = game_snake.tail;
  snake_body_part *new_tail =
      (snake_body_part *)malloc(sizeof(snake_body_part));
  new_tail->nodeid = tail->nodeid + 1;
  new_tail->x = tail->x;
  new_tail->y = tail->y;
  new_tail->next = NULL;
  tail->next = new_tail;
  game_snake.tail = new_tail;
}

void shutdown(screen_buffer buffer) {
  free_buffer(buffer);
  free_snake_game(game_snake);
}

int main() {
  int i = 0;
  screen_buffer buffer;
  buffer.width = 100;
  buffer.height = 50;
  printf("initializing bufffer\n");
  buffer = init_buffer(buffer);
  printf("Buffer initialized with widith: %d and height: %d\n", buffer.width,
         buffer.height);

  game_snake.head = (snake_body_part *)malloc(sizeof(snake_body_part));
  game_snake.head->x = buffer.width / 2;
  game_snake.head->y = buffer.height / 2;
  game_snake.head->vx = 1;
  game_snake.head->vy = 0;
  game_snake.head->nodeid = 0;
  game_snake.length = 3;

  snake_body_part *part = game_snake.head;
  i = 1;
  while (i < game_snake.length) {
    part->next = (snake_body_part *)malloc(sizeof(snake_body_part));
    part->next->x = game_snake.head->x - i;
    part->next->y = game_snake.head->y;
    part->next->nodeid = i;
    part = part->next;
    i++;
  }
  part->next = NULL;
  game_snake.tail = part;
  print_snake_coordinates(game_snake);
  enable_raw_mode();
  i = 0;
  while (1) {
    int key = read_key();
    switch (key) {
    case KEY_UP:
      if (game_snake.head->vy != 1) {
        game_snake.head->vx = 0;
        game_snake.head->vy = -1;
      }
      break;
    case KEY_DOWN:
      if (game_snake.head->vy != -1) {
        game_snake.head->vx = 0;
        game_snake.head->vy = 1;
      }
      break;
    case KEY_LEFT:
      if (game_snake.head->vx != 1) {
        game_snake.head->vx = -1;
        game_snake.head->vy = 0;
      }
      break;
    case KEY_RIGHT:
      if (game_snake.head->vx != -1) {

        game_snake.head->vx = 1;
        game_snake.head->vy = 0;
      }
      break;
    case 'q':
      free_buffer(buffer);
      free_snake_game(game_snake);
      return 0;
    }
    print_snake_coordinates(game_snake);
    int render_result = render(i++, buffer);
    if (render_result == -1) {
      shutdown(buffer);
      printf("game over\n");
      printf("shutdown complete\n");
      exit(0);
      break;
      return 0;
    }
    usleep(100000);
  }
  return 0;
}
