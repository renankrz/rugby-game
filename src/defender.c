// Standard headers
#include <stdio.h>
#include <stdlib.h>

// Internal headers
#include "direction.h"
#include "position.h"
#include "spy.h"

// Main header
#include "defender.h"

/*----------------------------------------------------------------------------*/
/*                          STRATEGIES DEFINITIONS                            */
/*----------------------------------------------------------------------------*/

typedef enum {
  STAY, ALIGN, LESS, FORWARD,
} StrategyType;

struct strategy {
  char *name;
  int rounds_left;
  int amplitude;
  int step;
  direction_t dir;
};
typedef struct strategy Strategy;

#define INIT_STAY     { "STAY", RAND_MAX, 0, 0, DIR_STAY }
#define INIT_ALIGN    { "ALIGN", -1, 0, 0, DIR_STAY }
#define INIT_LESS     { "LESS", RAND_MAX, 3, 0, DIR_STAY }
#define INIT_FORWARD  { "FORWARD", 0, 0, 0, DIR_LEFT }

const Strategy init_def_strategies[] = {
  INIT_STAY, INIT_ALIGN, INIT_LESS, INIT_FORWARD
};

/*----------------------------------------------------------------------------*/
/*                                    UTIL                                    */
/*----------------------------------------------------------------------------*/

// Functions defined elsewhere
extern void print_dir(direction_t dir);
extern bool is_same_dir(direction_t dir_1, direction_t dir_2);
extern void set_pos(position_t *target_pos, position_t source_pos);

void print_def(Strategy *s) {
  printf("--------------------\n");
  printf("[STRATEGY] %s\n", s->name);
  printf("%d rounds left\n", s->rounds_left);
  printf("%d amplitude\n", s->amplitude);
  printf("%d step\n", s->step);
  printf("dir: ");
  print_dir(s->dir);
  printf("--------------------\n");
}

/*----------------------------------------------------------------------------*/
/*                               STRATEGIES                                   */
/*----------------------------------------------------------------------------*/

void apply_stay(direction_t *dir) {
  *dir = (direction_t) DIR_STAY;
}

void apply_align(direction_t *dir, Strategy *s) {
  *dir = s->dir;
  s->rounds_left--;
}

void apply_less(direction_t *dir, Strategy *s) {
  if (s->step % 2 != 0) { // Change vertical direction in odd steps
    if (is_same_dir(s->dir, (direction_t) DIR_UP_RIGHT)
        || is_same_dir(s->dir, (direction_t) DIR_UP)) {
      s->dir = (direction_t) DIR_DOWN_LEFT;
    } else {
      s->dir = (direction_t) DIR_UP_LEFT;
    }
  } else { // Change horizontal direction in even steps
    if (is_same_dir(s->dir, (direction_t) DIR_UP)) {
      s->dir = (direction_t) DIR_UP;
    } else if (is_same_dir(s->dir, (direction_t) DIR_DOWN)) {
      s->dir = (direction_t) DIR_DOWN;
    } else if (is_same_dir(s->dir, (direction_t) DIR_UP_LEFT)) {
      s->dir = (direction_t) DIR_UP_RIGHT;
    } else {
      s->dir = (direction_t) DIR_DOWN_RIGHT;
    }
  }
  s->step = s->step + 1 > s->amplitude ? 0 : s->step + 1;
  *dir = s->dir;
}

void apply_forward(direction_t *dir, Strategy *s) {
  *dir = s->dir;
}

/*----------------------------------------------------------------------------*/
/*                              PUBLIC FUNCTIONS                              */
/*----------------------------------------------------------------------------*/

direction_t execute_defender_strategy(
    position_t current_pos, Spy attacker_spy) {
  // Game
  static int round = 1;

  // Strategy
  static StrategyType strategy_type = STAY;
  static Strategy strategy = INIT_STAY;

  // Lock management
  static direction_t last_dir;

  // Positions to check against current position
  static position_t initial_pos = INVALID_POSITION;
  static position_t last_pos = INVALID_POSITION;

  // Spy
  static position_t rival_pos;
  static bool already_spied = false;
  static int align_retries = 2;
  static bool aligned = false;

  // Return value
  direction_t dir;

  // Things to do only in the first round
  if (round == 1) {
    set_pos(&initial_pos, current_pos);
  }

  // Extract state of the game to meaningful variables
  bool is_locked = strategy_type != STAY && equal_positions(current_pos, last_pos);
  bool has_strategy_finished = strategy.rounds_left == 0;
  bool is_time_to_spy = !already_spied && round == 6;
  if (!aligned) {
    aligned = (int) current_pos.i - (int) rival_pos.i == 0;
  }
  bool retry_align = strategy_type == FORWARD && align_retries > 0 && !aligned;

  // Spy
  if (is_time_to_spy) {
    set_pos(&rival_pos, get_spy_position(attacker_spy));
    already_spied = true;
    int v_diff = (int) current_pos.i - (int) rival_pos.i;

    // Set new strategy
    if (v_diff != 0) { // Need to align
      strategy_type = ALIGN;
      strategy = (Strategy) INIT_ALIGN;
      strategy.rounds_left = abs(v_diff);
      strategy.dir = v_diff < 0 ? (direction_t) DIR_DOWN : (direction_t) DIR_UP;
    } else { // Pass through ALIGN
      strategy_type = LESS;
      strategy = (Strategy) INIT_LESS;
      strategy.dir = rand() % 2 == 0 ? (direction_t) DIR_UP : (direction_t) DIR_DOWN;
    }
  }

  if (has_strategy_finished) {
    if (retry_align) {
      int v_diff = (int) current_pos.i - (int) rival_pos.i;
      align_retries--;
      strategy_type = ALIGN;
      strategy = (Strategy) INIT_ALIGN;
      strategy.rounds_left = abs(v_diff);
      strategy.dir = v_diff < 0 ? (direction_t) DIR_DOWN : (direction_t) DIR_UP;
    } else {
      strategy_type = LESS;
      strategy = (Strategy) INIT_LESS;
      strategy.dir = rand() % 2 == 0 ? (direction_t) DIR_UP : (direction_t) DIR_DOWN;
    }
  }
  // Update strategy if we're stuck
  else if (is_locked) {
    if (rand() % 2 == 0) {
      strategy_type = FORWARD;
      strategy = (Strategy) INIT_FORWARD;
    } else {
      strategy_type = ALIGN;
      strategy = (Strategy) INIT_ALIGN;
      strategy.rounds_left = 2;
      if (is_same_dir(last_dir, (direction_t) DIR_UP)
          || is_same_dir(last_dir, (direction_t) DIR_UP_RIGHT)
          || is_same_dir(last_dir, (direction_t) DIR_UP_LEFT)) {
        strategy.dir = (direction_t) DIR_DOWN;
      } else {
        strategy.dir = (direction_t) DIR_UP;
      }
    }
  }

  // Apply the appropriate strategy
  switch (strategy_type) {
    case STAY:
      apply_stay(&dir);
      break;
    case ALIGN:
      apply_align(&dir, &strategy);
      break;
    case LESS:
      apply_less(&dir, &strategy);
      break;
    case FORWARD:
      apply_forward(&dir, &strategy);
      break;
    default:
      fprintf(stderr, "[ERROR] Strategy type %d doesn't exist.\n", strategy_type);
      break;
  }

  // Store current position
  set_pos(&last_pos, current_pos);

  // Store current direction
  last_dir = dir;

  round++;
  return dir;
}

/*----------------------------------------------------------------------------*/
