#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
int rand_number(int min, int max) { return (rand() % max) + min; }
long FRAMES = 0;
int score = 0;

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
typedef struct screen_buffer screen_buffer;
void populate_snake_food(screen_buffer buffer);
int draw_snake(screen_buffer buffer, SNAKE snake, int dt);

#define ASCII_LIB_KEY_HANDLER
#define GAME_RENDERING_FN                                                      \
  res = draw_snake(buffer, game_snake, dt);                                    \
  populate_snake_food(buffer);

#include "ascii_lib.h"

int draw_snake(screen_buffer buffer, SNAKE snake, int dt);

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
  DBG("p1 (%d;%d)", game_snake.head->x, game_snake.head->y);
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

void free_snake_game(SNAKE snake) {
  snake_body_part *part = snake.head;
  while (part != NULL) {
    snake_body_part *next = part->next;
    free(part);
    part = next;
  }
  printf("All snake body part free\n");
}

void print_snake_coordinates(SNAKE snake) {
  snake_body_part *part = game_snake.head;
  int i = 0;
  while (part != NULL) {
    printf("(%d;%d;%d)\n", part->nodeid, part->x, part->y);
    part = part->next;
    i++;
  }
}

int draw_snake(screen_buffer buffer, SNAKE snake, int dt) {
  snake_body_part *part = snake.head;
  int i = 0;
  printf("score: %d\n", score);
  int temp;
  int **node_coordinate = (int **)malloc(sizeof(int *) * snake.length);
  for (int j = 0; j < snake.length; j++) {
    if (part == NULL) {
      break;
    }
    node_coordinate[j] = (int *)malloc(sizeof(int) * 2);
    node_coordinate[j][0] = part->x;
    node_coordinate[j][1] = part->y;
    part = part->next;
  }

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

  free(node_coordinate);

  snake.head->x = snake.head->x + snake.head->vx;
  snake.head->y = snake.head->y + snake.head->vy;
  int val = check_collision(buffer);

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

int restart_game() {
  srand(time(NULL));
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
      return 0;
    }
    usleep(100000);
  }
}

int main() {
  while (1) {
    restart_game();

    disable_raw_mode();
    printf("game over\n");
    printf("Restart? (y/n)");

    char restart;
    scanf("%c", &restart);
    if (restart != 'y') {
      return 0;
    }
  };
  return 0;
}
