// Standard headers
#include <stdio.h>
#include <stdlib.h>

// Header to get time seed
#include <time.h>

// Internal headers
#include "direction.h"
#include "position.h"
#include "spy.h"

// Main header
#include "attacker.h"

// Macros
#define UNUSED(x) (void)(x) // Auxiliary to avoid error of unused parameter

/*----------------------------------------------------------------------------*/
/*                          STRATEGIES DEFINITIONS                            */
/*----------------------------------------------------------------------------*/

typedef enum {
  ZIGZAG, VERTICAL, TRIANGLE, SQUARE, INVALID_TYPE,
} StrategyType;

typedef enum {
  CLOCKWISE, COUNTERCLOCKWISE, RANDOM,
} Way;

struct strategy {
  char *name;
  int rounds_left;
  direction_t forbidden_dir;
  direction_t preferred_dir;
  Way way;
};
typedef struct strategy Strategy;

#define INIT_ZIGZAG   { "ZIGZAG", 0, DIR_STAY, DIR_STAY, RANDOM }
#define INIT_VERTICAL { "VERTICAL", 1, DIR_STAY, DIR_STAY, RANDOM }
#define INIT_TRIANGLE { "TRIANGLE", 4, DIR_STAY, DIR_STAY, RANDOM }
#define INIT_SQUARE   { "SQUARE", RAND_MAX, DIR_STAY, DIR_STAY, RANDOM }

const Strategy init_strategies[] = {
  INIT_ZIGZAG, INIT_VERTICAL, INIT_TRIANGLE, INIT_SQUARE
};

/*----------------------------------------------------------------------------*/
/*                                    UTIL                                    */
/*----------------------------------------------------------------------------*/

void print_dir(direction_t dir) {
  switch (dir.i) {
  case -1:
    switch (dir.j) {
      case -1:
        printf("up left\n");
        break;
      case 0:
        printf("up\n");
        break;
      case 1:
        printf("up right\n");
        break;
    }
    break;
  case 0:
    switch (dir.j) {
      case -1:
        printf("left\n");
        break;
      case 0:
        printf("stay\n");
        break;
      case 1:
        printf("right\n");
        break;
    }
    break;
  case 1:
    switch (dir.j) {
      case -1:
        printf("down left\n");
        break;
      case 0:
        printf("down\n");
        break;
      case 1:
        printf("down right\n");
        break;
    }
    break;
  default:
    printf("none\n");
  }
}

void print_pos(position_t pos) {
  printf("i = %lu | j = %lu\n", pos.i, pos.j);
}

void print_way(Way way) {
  if (way == CLOCKWISE) {
    printf("clockwise\n");
  } else {
    printf("counterclockwise\n");
  }
}

void print_strategy(Strategy *s) {
  printf("--------------------\n");
  printf("[STRATEGY] %s\n", s->name);
  printf("forbidden dir: ");
  print_dir(s->forbidden_dir);
  printf("way: ");
  print_way(s->way);
  printf("%d rounds left\n", s->rounds_left);
  printf("--------------------\n");
}

/*----------------------------------------------------------------------------*/
/*                            PRIVATE FUNCTIONS                               */
/*----------------------------------------------------------------------------*/

bool is_same_dir(direction_t dir_1, direction_t dir_2) {
  if (dir_1.i == dir_2.i && dir_1.j == dir_2.j)
    return true;
  return false;
}

void set_pos(position_t *target_pos, position_t source_pos) {
  target_pos->i = source_pos.i;
  target_pos->j = source_pos.j;
}

void set_dir(direction_t *target_dir, direction_t source_dir) {
  target_dir->i = source_dir.i;
  target_dir->j = source_dir.j;
}

void set_way(Way *way, direction_t dir) {
  if (is_same_dir(dir, (direction_t) DIR_UP)
    || is_same_dir(dir, (direction_t) DIR_UP_RIGHT)) {
    *way = CLOCKWISE;
  } else if (is_same_dir(dir, (direction_t) DIR_DOWN_RIGHT)
    || is_same_dir(dir, (direction_t) DIR_DOWN)) {
    *way = COUNTERCLOCKWISE;
  }
}

void set_rand_dir(direction_t *rand_dir, Way way) {
  int k = rand();
  if (way == CLOCKWISE) {
    if (k > RAND_MAX / 3) {
      // RIGHT a 67%
      set_dir(rand_dir, (direction_t) DIR_RIGHT);
    } else if (k > RAND_MAX / 6) {
      // UP_RIGHT a 17%
      set_dir(rand_dir, (direction_t) DIR_UP_RIGHT);
    } else {
      // UP a 17%
      set_dir(rand_dir, (direction_t) DIR_UP);
    }
  } else {
    if (k > RAND_MAX / 3) {
      // DOWN_RIGHT a 67%
      set_dir(rand_dir, (direction_t) DIR_RIGHT);
    } else if (k > RAND_MAX / 6) {
      // RIGHT a 17%
      set_dir(rand_dir, (direction_t) DIR_DOWN_RIGHT);
    } else {
      // DOWN a 17%
      set_dir(rand_dir, (direction_t) DIR_DOWN);
    }
  }
}

void update_strategy_type(
    StrategyType *type, StrategyType *max, bool got_locked_recently) {
  if (got_locked_recently) {
    *type = *max + 1 == INVALID_TYPE ? ZIGZAG : *max + 1;
  } else {
    *type = *type + 1 == INVALID_TYPE ? ZIGZAG : *type + 1;
  }
  // Update max
  if (*type > *max) {
    *max = *type;
  }
}

/*----------------------------------------------------------------------------*/
/*                               STRATEGIES                                   */
/*----------------------------------------------------------------------------*/

void apply_zigzag(
    direction_t *dir, Strategy *s) {

  // Treat preferred direction
  if (!is_same_dir(s->preferred_dir, (direction_t) DIR_STAY)) {
    set_dir(dir, s->preferred_dir);
  } else {
    // Resolve random way
    if (s->way == RANDOM) {
      s->way = rand() % 2 == 0 ? CLOCKWISE : COUNTERCLOCKWISE;
    }

    // Go the same way with probability 7/8
    if (rand() % 8 != 0) {
      // Keep going
      set_rand_dir(dir, s->way);
    } else {
      // Change way
      if (s->way == CLOCKWISE) {
        set_dir(dir, (direction_t) DIR_DOWN_RIGHT);
        s->way = COUNTERCLOCKWISE;
      } else {
        set_dir(dir, (direction_t) DIR_UP_RIGHT);
        s->way = CLOCKWISE;
      }
    }
  }
}

void apply_vertical(
  direction_t *dir, Strategy *s) {
  // Treat preferred direction
  if (!is_same_dir(s->preferred_dir, (direction_t) DIR_STAY)) {
    set_dir(dir, s->preferred_dir);
  } else {
    if (is_same_dir(s->forbidden_dir, (direction_t) DIR_UP)) {
      set_dir(dir, (direction_t) DIR_DOWN);
    } else if (is_same_dir(s->forbidden_dir, (direction_t) DIR_DOWN)) {
      set_dir(dir, (direction_t) DIR_UP);
    } else if (s->way == CLOCKWISE) {
      set_dir(dir, (direction_t) DIR_UP);
    } else {
      set_dir(dir, (direction_t) DIR_DOWN);
    }
  }
  s->rounds_left--;
}

void apply_triangle(
  direction_t *dir, Strategy *s) {
  if (s->way == CLOCKWISE) {
    if (s->rounds_left > 2) {
      set_dir(dir, (direction_t) DIR_UP_LEFT);
    } else {
      set_dir(dir, (direction_t) DIR_UP_RIGHT);
    }
  } else {
    if (s->rounds_left > 2) {
      set_dir(dir, (direction_t) DIR_DOWN_LEFT);
    } else {
      set_dir(dir, (direction_t) DIR_DOWN_RIGHT);
    }
  }
  s->rounds_left--;
}

void apply_square(
    direction_t *dir, Strategy *s, bool is_locked) {
  static int step = 0;
  static int squares_away = 0;
  if (is_locked) {
    if (step == 0) {
      if (s->way == CLOCKWISE) {
        set_dir(dir, (direction_t) DIR_UP);
      } else {
        set_dir(dir, (direction_t) DIR_DOWN);
      }
      step++;
    } else if (step == 1) {
      set_dir(dir, (direction_t) DIR_RIGHT);
      squares_away--;
      step++;
    } else {
      set_rand_dir(dir, s->way);
      step = 0;
      s->rounds_left = 0;
    }
  } else {
    if (step == 0) {
      set_dir(dir, (direction_t) DIR_LEFT);
      squares_away++;
    } else if (step == 1) {
      if (s->way == CLOCKWISE) {
        set_dir(dir, (direction_t) DIR_UP);
      } else {
        set_dir(dir, (direction_t) DIR_DOWN);
      }
    } else {
      if (squares_away > 0) {
        set_dir(dir, (direction_t) DIR_RIGHT);
        squares_away--;
      } else {
        set_rand_dir(dir, s->way);
        step = 0;
        s->rounds_left = 0;
      }
    }
  }
}

/*----------------------------------------------------------------------------*/
/*                              PUBLIC FUNCTIONS                              */
/*----------------------------------------------------------------------------*/

direction_t execute_attacker_strategy(
    position_t current_pos, Spy defender_spy) {
  UNUSED(current_pos);
  UNUSED(defender_spy);

  // Game
  static int round = 1;

  // Strategy
  static StrategyType strategy_type = ZIGZAG;
  static StrategyType max_strategy_type = ZIGZAG;
  static Strategy strategy = INIT_ZIGZAG;
  static Way way = RANDOM;

  // Lock management
  static int rounds_free = 0;
  static direction_t last_dir;

  // Positions to check against current position
  static position_t initial_pos = INVALID_POSITION;
  static position_t last_pos = INVALID_POSITION;

  // Spy
  static bool already_spied = false;
  static int rounds_since_spy = 0;
  static direction_t preferred_dir = DIR_STAY;

  // Return value
  direction_t dir;

  // Things to do only in the first round
  if (round == 1) {
    srand(time(NULL));
    set_pos(&initial_pos, current_pos);
  }

  // Extract state of the game to meaningful variables
  bool is_locked = equal_positions(current_pos, last_pos);
  bool got_locked_recently = rounds_free < 5;
  rounds_free = is_locked ? 0 : rounds_free + 1;
  bool has_strategy_finished = strategy_type != ZIGZAG && strategy.rounds_left == 0;
  bool is_time_to_spy = !already_spied && (current_pos.j == 6 || round == 30);

  // Spy
  if (is_time_to_spy) {
    position_t rival_pos;
    set_pos(&rival_pos, get_spy_position(defender_spy));
    print_pos(rival_pos);
    already_spied = true;
    int h_diff = (int) current_pos.j - (int) rival_pos.j;
    int v_diff = (int) current_pos.i - (int) rival_pos.i;
    int v_displacement = (int) current_pos.i - (int) initial_pos.i;

    if (abs(h_diff) + abs(v_diff) <= 6) { // We're close enough to care
      if (h_diff > 0) { // We passed
        preferred_dir = (direction_t) DIR_RIGHT;
      } else if (h_diff == 0) { // We're passing
        if (v_diff == -1) { // We're up
          preferred_dir = (direction_t) DIR_UP_RIGHT;
        } else if (v_diff == 1) { // We're down
          preferred_dir = (direction_t) DIR_DOWN_RIGHT;
        } else { // We're vertically far
          preferred_dir = (direction_t) DIR_RIGHT;
        }
      } else if (v_diff <= -2) { // We're behind and up
        preferred_dir = (direction_t) DIR_UP_RIGHT;
      } else if (v_diff == -1) { // We're behind and up
        if (abs(h_diff) <= 3) { // We're behind, up and close
          preferred_dir = (direction_t) DIR_UP;
        } else { // We're behind, up and far
          preferred_dir = (direction_t) DIR_UP_RIGHT;
        }
      } else if (v_diff == 0) { // We're behind and aligned
        if (abs(h_diff) <= 3) { // We're behind, aligned and close
          if (v_displacement < 0) { // We've gone up
            preferred_dir = (direction_t) DIR_DOWN_LEFT;
          } else {  // We've gone down
            preferred_dir = (direction_t) DIR_UP_LEFT;  
          }
        } else { // We're behind, aligned and far
          if (v_displacement < 0) { // We've gone up
            preferred_dir = (direction_t) DIR_DOWN_RIGHT;
          } else {  // We've gone down
            preferred_dir = (direction_t) DIR_UP_RIGHT;
          }
        }
      } else if (v_diff == 1) { // We're behind and down
        if (abs(h_diff) <= 3) { // We're behind, down and close
          preferred_dir = (direction_t) DIR_DOWN;
        } else { // We're behind, down and far
          preferred_dir = (direction_t) DIR_DOWN_RIGHT;
        }
      } else if (v_diff >= 2) { // We're behind and down
        preferred_dir = (direction_t) DIR_DOWN_RIGHT;
      }
    }
  }

  if (already_spied) {
    // Expire preferred_dir sometime
    rounds_since_spy++;
    if (!is_same_dir(preferred_dir, (direction_t) DIR_STAY)
        && rounds_since_spy == 2) {
      preferred_dir = (direction_t) DIR_STAY;
    }
    // Enforce spy strategy or its expiration
    strategy.preferred_dir = preferred_dir;
  }

  // Return to the default strategy
  if (has_strategy_finished) {
    if (strategy_type == SQUARE) {
      // Reset lock auxiliaries
      got_locked_recently = false;
      max_strategy_type = ZIGZAG;
    }
    // Reset strategy   
    strategy_type = ZIGZAG;
    strategy = (Strategy) INIT_ZIGZAG;
    strategy.preferred_dir = preferred_dir;
    strategy.way = way;
  }
  // Update strategy if we're stuck
  else if (is_locked && strategy_type != SQUARE) {
    update_strategy_type(&strategy_type, &max_strategy_type, got_locked_recently);
    strategy = init_strategies[strategy_type];
    strategy.forbidden_dir = last_dir;
    strategy.preferred_dir = preferred_dir;
    strategy.way = way == CLOCKWISE ? COUNTERCLOCKWISE : CLOCKWISE;
    // Reset is_locked to use with SQUARE strategy
    is_locked = false;
  }

  // Apply the appropriate strategy
  switch (strategy_type) {
    case ZIGZAG:
      apply_zigzag(&dir, &strategy);
      break;
    case VERTICAL:
      apply_vertical(&dir, &strategy);
      break;
    case TRIANGLE:
      apply_triangle(&dir, &strategy);
      break;
    case SQUARE:
      apply_square(&dir, &strategy, is_locked);
      break;
    default:
      fprintf(stderr, "[ERROR] Strategy type %d doesn't exist.\n", strategy_type);
      break;
  }

  // Store current position
  set_pos(&last_pos, current_pos);

  // Store current direction
  set_dir(&last_dir, dir);

  // Store movement way (clockwise or counterclockwise)
  set_way(&way, dir);

  round++;

  return dir;
}
