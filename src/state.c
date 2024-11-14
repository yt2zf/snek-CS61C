#include "state.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "snake_utils.h"

/* Helper function definitions */
static void set_board_at(game_state_t *state, unsigned int row, unsigned int col, char ch);
static bool is_tail(char c);
static bool is_head(char c);
static bool is_snake(char c);
static char body_to_tail(char c);
static char head_to_body(char c);
static unsigned int get_next_row(unsigned int cur_row, char c);
static unsigned int get_next_col(unsigned int cur_col, char c);
static void find_head(game_state_t *state, unsigned int snum);
static char next_square(game_state_t *state, unsigned int snum);
static void update_tail(game_state_t *state, unsigned int snum);
static void update_head(game_state_t *state, unsigned int snum);

/* Task 1 */
game_state_t *create_default_state()
{
  game_state_t *defaultState = malloc(sizeof(game_state_t));
  if (defaultState == NULL)
  {
    return NULL;
  }
  // set up snake
  defaultState->num_snakes = 1;
  defaultState->snakes = malloc(sizeof(snake_t));
  if (defaultState->snakes == NULL)
  {
    free(defaultState);
    return NULL;
  }

  defaultState->snakes[0].head_row = 2;
  defaultState->snakes[0].head_col = 4;
  defaultState->snakes[0].tail_row = 2;
  defaultState->snakes[0].tail_col = 2;
  defaultState->snakes[0].live = true;

  // set up board
  unsigned num_rows = 18;
  unsigned num_cols = 20;

  defaultState->num_rows = num_rows;
  defaultState->board = malloc(num_rows * sizeof(char *));
  if (defaultState->board == NULL)
  {
    free(defaultState->snakes);
    free(defaultState);
    return NULL;
  }

  for (int i = 0; i < num_rows; i++)
  {
    defaultState->board[i] = malloc((num_cols + 2) * sizeof(char));
    if (defaultState->board[i] == NULL)
    {
      for (int j = 0; j < i; j++)
      {
        free(defaultState->board[j]);
      }
      free(defaultState->board);
      free(defaultState->snakes);
      free(defaultState);
      return NULL;
    }
  }

  char upper[] = "####################\n";
  char middle[] = "#                  #\n";
  char bottom[] = "####################\n";

  strncpy(defaultState->board[0], upper, num_cols + 2);
  strncpy(defaultState->board[num_rows - 1], bottom, num_cols + 2);
  for (int i = 1; i < num_rows - 1; i++)
  {
    strncpy(defaultState->board[i], middle, num_cols + 2);
  }

  // set up fruit
  defaultState->board[2][9] = '*';

  defaultState->board[2][2] = 'd';
  defaultState->board[2][3] = '>';
  defaultState->board[2][4] = 'D';
  return defaultState;
}

/* Task 2 */
void free_state(game_state_t *state)
{
  if (state == NULL)
  {
    return;
  }
  if (state->snakes != NULL)
  {
    free(state->snakes);
  }
  if (state->board != NULL)
  {
    for (int i = 0; i < state->num_rows; i++)
    {
      free(state->board[i]);
    }
    free(state->board);
  }
  free(state);
}

/* Task 3 */
void print_board(game_state_t *state, FILE *fp)
{
  for (int i = 0; i < state->num_rows; i++)
  { 
    fprintf(fp, "%s", state->board[i]);
  }
}

/*
  Saves the current state into filename. Does not modify the state object.
  (already implemented for you).
*/
void save_board(game_state_t *state, char *filename)
{
  FILE *f = fopen(filename, "w");
  print_board(state, f);
  fclose(f);
}

/* Task 4.1 */

/*
  Helper function to get a character from the board
  (already implemented for you).
*/
char get_board_at(game_state_t *state, unsigned int row, unsigned int col) { return state->board[row][col]; }

/*
  Helper function to set a character on the board
  (already implemented for you).
*/
static void set_board_at(game_state_t *state, unsigned int row, unsigned int col, char ch)
{
  state->board[row][col] = ch;
}

/*
  Returns true if c is part of the snake's tail.
  The snake consists of these characters: "wasd"
  Returns false otherwise.
*/
static bool is_tail(char c)
{
  switch (c)
  {
  case 'w':
  case 'a':
  case 's':
  case 'd':
    return true;
    break;
  default:
    return false;
    break;
  }
}

/*
  Returns true if c is part of the snake's head.
  The snake consists of these characters: "WASDx"
  Returns false otherwise.
*/
static bool is_head(char c)
{
  switch (c)
  {
  case 'W':
  case 'A':
  case 'S':
  case 'D':
  case 'x':
    return true;
    break;
  default:
    return false;
    break;
  }
}

/*
  Returns true if c is part of the snake.
  The snake consists of these characters: "wasd^<v>WASDx"
*/
static bool is_snake(char c)
{
  if (is_head(c) || is_tail(c))
  {
    return true;
  }
  switch (c)
  {
  case '^':
  case '<':
  case 'v':
  case '>':
    return true;
    break;
  default:
    return false;
    break;
  }
}

/*
  Converts a character in the snake's body ("^<v>")
  to the matching character representing the snake's
  tail ("wasd").
*/
static char body_to_tail(char c)
{
  switch (c)
  {
  case '^':
    return 'w';
    break;
  case '<':
    return 'a';
    break;
  case 'v':
    return 's';
    break;
  case '>':
    return 'd';
    break;
  default:
    return '?';
    break;
  }
}

/*
  Converts a character in the snake's head ("WASD")
  to the matching character representing the snake's
  body ("^<v>").
*/
static char head_to_body(char c)
{
  switch (c)
  {
  case 'W':
    return '^';
    break;
  case 'A':
    return '<';
    break;
  case 'S':
    return 'v';
    break;
  case 'D':
    return '>';
    break;
  default:
    return '?';
    break;
  }
}

/*
  Returns cur_row + 1 if c is 'v' or 's' or 'S'.
  Returns cur_row - 1 if c is '^' or 'w' or 'W'.
  Returns cur_row otherwise.
*/
static unsigned int get_next_row(unsigned int cur_row, char c)
{
  switch (c)
  {
  case 'v':
  case 's':
  case 'S':
    return cur_row + 1;
    break;
  case '^':
  case 'w':
  case 'W':
    return cur_row - 1;
    break;
  default:
    return cur_row;
  }
}

/*
  Returns cur_col + 1 if c is '>' or 'd' or 'D'.
  Returns cur_col - 1 if c is '<' or 'a' or 'A'.
  Returns cur_col otherwise.
*/
static unsigned int get_next_col(unsigned int cur_col, char c)
{
  switch (c)
  {
  case '>':
  case 'd':
  case 'D':
    return cur_col + 1;
    break;
  case '<':
  case 'a':
  case 'A':
    return cur_col - 1;
    break;
  default:
    return cur_col;
  }
}

/*
  Task 4.2

  Helper function for update_state. Return the character in the cell the snake is moving into.

  This function should not modify anything.
*/
static char next_square(game_state_t *state, unsigned int snum)
{
  snake_t *snake = state->snakes + snum;
  char c = get_board_at(state, snake->head_row, snake->head_col);
  unsigned nextRow = get_next_row(snake->head_row, c);
  unsigned nextCol = get_next_col(snake->head_col, c);

  return get_board_at(state, nextRow, nextCol);
}

/*
  Task 4.3

  Helper function for update_state. Update the head...

  ...on the board: add a character where the snake is moving

  ...in the snake struct: update the row and col of the head

  Note that this function ignores food, walls, and snake bodies when moving the head.
*/
static void update_head(game_state_t *state, unsigned int snum)
{
  snake_t *snake = state->snakes + snum;
  char c = get_board_at(state, snake->head_row, snake->head_col);
  unsigned nextRow = get_next_row(snake->head_row, c);
  unsigned nextCol = get_next_col(snake->head_col, c);

  // On the game board, add a character where the snake is moving
  state->board[nextRow][nextCol] = c;
  state->board[snake->head_row][snake->head_col] = head_to_body(c);
  snake->head_row = nextRow;
  snake->head_col = nextCol;
}

/*
  Task 4.4

  Helper function for update_state. Update the tail...

  ...on the board: blank out the current tail, and change the new
  tail from a body character (^<v>) into a tail character (wasd)

  ...in the snake struct: update the row and col of the tail
*/
static void update_tail(game_state_t *state, unsigned int snum)
{
  snake_t *snake = state->snakes + snum;
  char c = get_board_at(state, snake->tail_row, snake->tail_col);
  unsigned nextRow = get_next_row(snake->tail_row, c);
  unsigned nextCol = get_next_col(snake->tail_col, c);

  // On the game board, add a character where the snake is moving
  c = get_board_at(state, nextRow, nextCol);
  state->board[nextRow][nextCol] = body_to_tail(c);
  state->board[snake->tail_row][snake->tail_col] = ' ';
  snake->tail_row = nextRow;
  snake->tail_col = nextCol;
}

/* Task 4.5 */
void update_state(game_state_t *state, int (*add_food)(game_state_t *state))
{
  for (unsigned i = 0; i < state->num_snakes; i++)
  {
    char nextC = next_square(state, i);
    snake_t *snake = state->snakes + i;
    if (snake->live)
    {
      if (nextC == '#' || is_snake(nextC))
      {
        snake->live = false;
        set_board_at(state, snake->head_row, snake->head_col, 'x');
      }
      else if (nextC == '*')
      {
        update_head(state, i);
        add_food(state);
      }
      else
      {
        update_head(state, i);
        update_tail(state, i);
      }
    }
  }
}

/* Task 5.1 */
char *read_line(FILE *fp)
{ 
  size_t capacity = 128;
  char *line = malloc(capacity * sizeof(char));
  if (line == NULL)
  {
    return NULL;
  }
  if (fgets(line, capacity, fp) == NULL)
  {
    free(line);
    return NULL;
  }

  
  char *newLine = memchr(line, '\n', capacity - 1);
  while (newLine == NULL){
    // fgets doesnt meet \n
    size_t oldCap = capacity;
    capacity += (capacity + 1) / 2;
    line = realloc(line, sizeof(char) * capacity);
    
    // line[old capacity-1] = '\0'; 
    // fgets to cover index from old capacity - 1 to new capacity - 1
    // n = old + 2
    char *concatStart = line + (oldCap -1);
    fgets(concatStart, capacity - oldCap + 1, fp);
    newLine = memchr(concatStart, '\n', capacity - oldCap);
  }

  // 根据字符的实际长度缩小空间
  size_t actualLen = newLine - line + 2;
  line = realloc(line, sizeof(char) * actualLen);
  return line;
}

/* Task 5.2 */
game_state_t *load_board(FILE *fp)
{
  game_state_t *gameState = malloc(sizeof(game_state_t));
  if (gameState == NULL)
  {
    return NULL;
  }
  gameState->num_snakes = 0;
  gameState->num_rows = 0;
  gameState->board = NULL;
  gameState->snakes = NULL;

  char *line;
  unsigned lineNum = 0;
  size_t capacity = 1;
  gameState->board = malloc(sizeof(char *) * capacity);
  if (gameState->board == NULL)
  {
    free(gameState);
    return NULL;
  }

  while ((line = read_line(fp)) != NULL)
  {
    if (lineNum >= capacity)
    {
      capacity += (capacity + 1) / 2;
      char **newBoard = realloc(gameState->board, sizeof(char *) * capacity);
      if (newBoard == NULL)
      {
        for (int i = 0; i < lineNum; i++)
        {
          free(gameState->board[i]);
        }
        free(gameState->board);
        free(gameState);
        return NULL;
      }
      gameState->board = newBoard;
    }
    gameState->board[lineNum] = line;
    lineNum++;
  }
  gameState->num_rows = lineNum;
  // 如果capacity > lineNum; 缩小为state->board分配的内存空间
  if (capacity > lineNum){
    gameState->board = realloc(gameState->board, sizeof(char *) * lineNum);
  } 
  return gameState;
}

/*
  Task 6.1

  Helper function for initialize_snakes.
  Given a snake struct with the tail row and col filled in,
  trace through the board to find the head row and col, and
  fill in the head row and col in the struct.
*/
static void find_head(game_state_t *state, unsigned int snum)
{
  snake_t *snake = state->snakes + snum;

  unsigned curRow = snake->tail_row;
  unsigned curCol = snake->tail_col;
  char curC = get_board_at(state, curRow, curCol);

  while (!is_head(curC))
  {
    curRow = get_next_row(curRow, curC);
    curCol = get_next_col(curCol, curC);
    curC = get_board_at(state, curRow, curCol);
  }

  snake->head_row = curRow;
  snake->head_col = curCol;

  return;
}

/* Task 6.2 */
game_state_t *initialize_snakes(game_state_t *state)
{ 
  if (state == NULL){
    return NULL;
  }
  size_t capacity = 1;
  state->snakes = malloc(sizeof(snake_t) * capacity);
  if (state->snakes == NULL)
  {
    free_state(state);
    return NULL;
  }

  unsigned numSnakes = 0;
  for (unsigned i = 0; i < state->num_rows; i++)
  {
    for (unsigned j = 0; j < strlen(state->board[i]) - 1; j++)
    {
      if (is_tail(state->board[i][j]))
      {
        if (numSnakes >= capacity)
        {
          capacity += (capacity + 1) / 2;
          snake_t *newSnakes = realloc(state->snakes, sizeof(snake_t) * capacity);
          if (newSnakes == NULL)
          {
            free_state(state);
            return NULL;
          }
          state->snakes = newSnakes;
        }

        state->snakes[numSnakes].live = true;
        state->snakes[numSnakes].tail_row = i;
        state->snakes[numSnakes].tail_col = j;
        find_head(state, numSnakes);
        numSnakes++;
      }
    }
  }
  state->num_snakes = numSnakes;
  // 如果capacity > numSnakes; 缩小为state->snakes分配的内存空间
  if (capacity > numSnakes){
    state->snakes = realloc(state->snakes, sizeof(snake_t) * numSnakes);
  }

  return state;
}
