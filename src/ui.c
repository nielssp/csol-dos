/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#include <stdlib.h>
#include <curses.h>
#include <locale.h>
#include <time.h>

#include "ui.h"
#include "card.h"
#include "theme.h"

#define COLOR_PAIR_BACKGROUND 1
#define COLOR_PAIR_EMPTY 2
#define COLOR_PAIR_BACK 3
#define COLOR_PAIR_RED 4
#define COLOR_PAIR_BLACK 5

int deals = 0;

int cur_x = 0;
int cur_y = 0;

int max_x = 0;
int max_y = 0;

int win_w = 0;
int win_h = 0;

int off_y = 0;

Card *selection = NULL;
Pile *selection_pile = NULL;
Card *cursor_card = NULL;
Pile *cursor_pile = NULL;

void print_card_name_l(int y, int x, Card *card, Theme *theme) {
  if (y < 0 || y >= win_h) {
    return;
  }
  switch (card->rank) {
    case ACE:
      mvprintw(y, x, "A");
      break;
    case KING:
      mvprintw(y, x, "K");
      break;
    case QUEEN:
      mvprintw(y, x, "Q");
      break;
    case JACK:
      mvprintw(y, x, "J");
      break;
    default:
      mvprintw(y, x, "%d", card->rank);
  }
  raw_output(1);
  printw(card_suit(card, theme));
  raw_output(0);
}

void print_card_name_r(int y, int x, Card *card, Theme *theme) {
  if (y < 0 || y >= win_h) {
    return;
  }
  raw_output(1);
  mvprintw(y, x - 1 - (card->rank == 10), card_suit(card, theme));
  raw_output(0);
  switch (card->rank) {
    case ACE:
      printw("A");
      break;
    case KING:
      printw("K");
      break;
    case QUEEN:
      printw("Q");
      break;
    case JACK:
      printw("J");
      break;
    default:
      printw("%d", card->rank);
  }
}

void print_layout(int y, int x, Card *card, Layout layout, int full, Theme *theme) {
  if (y >= win_h) {
    return;
  }
  if (y >= 0) {
    mvprintw(y, x, layout.top);
  }
  if (full && theme->height > 1) {
    int i;
    for (i = 1; i < theme->height - 1; i++) {
      if (y + i >= 0 && y + i < win_h) {
        mvprintw(y + i, x, layout.middle);
      }
    }
    if (y + theme->height > 0 && y + theme->height <= win_h) {
      mvprintw(y + theme->height - 1, x, layout.bottom);
    }
  }
}

int print_card(int y, int x, Card *card, int full, Theme *theme) {
  int y2 = y + full * (theme->height - 1);
  if (y2 > max_y) max_y = y2;
  if (x > max_x) max_x = x;
  if (y <= cur_y && y2 >= cur_y && x == cur_x) cursor_card = card;
  card->x = x;
  card->y = y;
  y = theme->y_margin + off_y + y;
  x = theme->x_margin + x * (theme->width + theme->x_spacing);
  if (win_h - 1 < y) {
    return 0;
  }
  if (card == selection) {
    attron(A_REVERSE);
  }
  if (card->suit & BOTTOM) {
    attron(COLOR_PAIR(COLOR_PAIR_EMPTY));
    print_layout(y, x, card, theme->empty_layout, full, theme);
    if (card->rank > 0) {
      print_card_name_l(y, x + 1, card, theme);
    }
  } else if (!card->up) {
    attron(COLOR_PAIR(COLOR_PAIR_BACK));
    print_layout(y, x, card, theme->back_layout, full, theme);
  } else {
    if (card->suit & RED) {
      attron(COLOR_PAIR(COLOR_PAIR_RED));
      print_layout(y, x, card, theme->red_layout, full, theme);
    } else {
      attron(COLOR_PAIR(COLOR_PAIR_BLACK));
      print_layout(y, x, card, theme->black_layout, full, theme);
    }
    if (full && theme->height > 1) {
      print_card_name_r(y + theme->height - 1, x + theme->width - 2, card, theme);
    }
    print_card_name_l(y, x + 1, card, theme);
  }
  attroff(A_REVERSE);
  return cursor_card == card;
}

int print_card_top(int y, int x, Card *card, Theme *theme) {
  return print_card(y, x, card, 0, theme);
}

int print_card_full(int y, int x, Card *card, Theme *theme) {
  return print_card(y, x, card, 1, theme);
}

int print_stack(int y, int x, Card *bottom, Theme *theme) {
  if (bottom->next) {
    return print_stack(y, x, bottom->next, theme);
  } else {
    return print_card_full(y, x, bottom, theme);
  }
}

int print_tableau(int y, int x, Card *bottom, Theme *theme) {
  if (bottom->next && bottom->suit & BOTTOM) {
    return print_tableau(y, x, bottom->next, theme);
  } else {
    if (bottom->next) {
      int cursor_below = print_card_top(y, x, bottom, theme);
      return print_tableau(y + 1, x, bottom->next, theme) || cursor_below;
    } else {
      int cursor_below = cur_x == x && cur_y >= y;
      return print_card_full(y, x, bottom, theme) || cursor_below;
    }
  }
}


void print_pile(Pile *pile, Theme *theme) {
  int y = pile->rule->y * (theme->height + theme->y_spacing);
  if (pile->rule->type == RULE_STOCK) {
    Card *top = get_top(pile->stack);
    top->up = 0;
    if (print_card_full(y, pile->rule->x, top, theme)) {
      cursor_pile = pile;
    }
  } else if (pile->stack->suit == TABLEAU) {
    if (print_tableau(y, pile->rule->x, pile->stack, theme)) {
      cursor_pile = pile;
    }
  } else if (print_stack(y, pile->rule->x, pile->stack, theme)) {
    cursor_pile = pile;
  }
}

int ui_loop(Game *game, Theme *theme, Pile *piles) {
  MEVENT mouse;
  int mouse_action = 0;
  selection = NULL;
  selection_pile = NULL;
  clear();
  clear_undo_history();
  move_counter = 0;
  off_y = 0;
  wbkgd(stdscr, COLOR_PAIR(COLOR_PAIR_BACKGROUND));
  while (1) {
    Pile *pile;
    int ch;
    cursor_card = NULL;
    cursor_pile = NULL;
    max_x = max_y = 0;
    getmaxyx(stdscr, win_h, win_w);
    if (theme->y_margin + off_y + cur_y >= win_h) {
      clear();
      off_y = win_h - cur_y - theme->y_margin - 1;
    }
    if (theme->y_margin + off_y + cur_y < 0) {
      clear();
      off_y = -theme->y_margin - cur_y;
      if (cur_y == 0) {
        off_y = 0;
      }
    }
    for (pile = piles; pile; pile = pile->next) {
      print_pile(pile, theme);
    }
    move(theme->y_margin + off_y + cur_y, theme->x_margin + cur_x * (theme->width + theme->x_spacing));
    refresh();

    attron(COLOR_PAIR(COLOR_PAIR_BACKGROUND));
    if (mouse_action) {
      ch = mouse_action;
      mouse_action = 0;
    } else {
      ch = getch();
    }
    switch (ch) {
      case 'h':
      case KEY_LEFT:
        cur_x--;
        if (cur_x < 0) cur_x = 0;
        break;
      case 'j':
      case KEY_DOWN:
        cur_y++;
        if (cur_y > max_y) cur_y = max_y;
        break;
      case 'k':
      case KEY_UP:
        cur_y--;
        if (cur_y < 0) cur_y = 0;
        break;
      case 'l':
      case KEY_RIGHT:
        cur_x++;
        if (cur_x > max_x) cur_x = max_x;
        break;
      case 'K':
      case 337: /* shift-up */
        if (cursor_card) {
          if (cursor_card->prev && NOT_BOTTOM(cursor_card->prev)) {
            Card *card = cursor_card->prev;
            while (card->prev && card->prev->up == card->up && NOT_BOTTOM(card->prev)) {
              card = card->prev;
            }
            cur_y = card->y;
          } else {
            cur_y -= theme->height;
          }
        } else if (cursor_pile) {
          Card *card = get_top(cursor_pile->stack);
          cur_y = card->y;
        } else {
          cur_y -= theme->height;
        }
        if (cur_y < 0) cur_y = 0;
        break;
      case 'J':
      case 336: /* shift-down */
        if (cursor_card && cursor_card->next) {
          Card *card = cursor_card->next;
          while (card->next && card->next->up == card->up) {
            card = card->next;
          }
          cur_y = card->y;
        } else {
          cur_y += theme->height;
        }
        if (cur_y > max_y) cur_y = max_y;
        break;
      case ' ':
        if (cursor_card) {
          if (!(cursor_card->suit & BOTTOM)) {
            if (cursor_card->up) {
              if (selection == cursor_card) {
                if (move_to_foundation(cursor_card, cursor_pile, piles) || move_to_free_cell(cursor_card, cursor_pile, piles)) {
                  clear();
                  selection = NULL;
                  selection_pile = NULL;
                }
              } else {
                selection = cursor_card;
                selection_pile = cursor_pile;
              }
            } else if (cursor_pile->rule->type == RULE_STOCK) {
              if (move_to_waste(cursor_card, cursor_pile, piles)) {
                clear();
              }
            } else {
              turn_card(cursor_card);
            }
          } else if (cursor_pile->rule->type == RULE_STOCK) {
            if (redeal(cursor_pile, piles)) {
              clear();
            } else {
              mvprintw(0, 0, "no more redeals");
            }
          }
        }
        break;
      case 'm':
      case 10: /* enter */
      case 13: /* enter */
        if (selection && cursor_pile) {
          if (legal_move_stack(cursor_pile, selection, selection_pile)) {
            clear();
            selection = NULL;
            selection_pile = NULL;
          }
        }
        break;
      case 'a':
        if (auto_move_to_foundation(piles)) {
          clear();
        }
        break;
      case 'u':
      case 26: /* ^z */
        undo_move();
        clear();
        break;
      case 'U':
      case 25: /* ^y */
        redo_move();
        clear();
        break;
      case 27:
        clear();
        selection = NULL;
        selection_pile = NULL;
        break;
      case KEY_RESIZE:
        clear();
        break;
      case 'r':
        return 1;
      case 'q':
        return 0;
      case KEY_MOUSE:
        if (nc_getmouse(&mouse) == OK) {
          cur_y = mouse.y - theme->y_margin - off_y;
          cur_x = (mouse.x - theme->x_margin) / (theme->width + theme->x_spacing);
          if (mouse.bstate & BUTTON3_PRESSED) {
            mouse_action = 'm';
          } else {
            mouse_action = ' ';
          }
        }
        break;
      default:
        mvprintw(0, 0, "%d", ch);
    }
  }
  return 0;
}

void ui_main(Game *game, Theme *theme, int enable_color, unsigned int seed) {
  mmask_t oldmask;
  setlocale(LC_ALL, "");
  initscr();
  if (enable_color) {
    Color *color;
    start_color();
    for (color = theme->colors; color; color = color->next) {
      if (init_color(color->index, color->red, color->green, color->blue) != 0) {
        /* TODO: inform user */
      }
    }
    init_pair(COLOR_PAIR_BACKGROUND, theme->background.fg, theme->background.bg);
    init_pair(COLOR_PAIR_EMPTY, theme->empty_layout.color.fg, theme->empty_layout.color.bg);
    init_pair(COLOR_PAIR_BACK, theme->back_layout.color.fg, theme->back_layout.color.bg);
    init_pair(COLOR_PAIR_RED, theme->red_layout.color.fg, theme->red_layout.color.bg);
    init_pair(COLOR_PAIR_BLACK, theme->black_layout.color.fg, theme->black_layout.color.bg);
  }
  raw();
  clear();
  curs_set(1);
  keypad(stdscr, 1);
  noecho();

  mousemask(BUTTON1_PRESSED | BUTTON3_PRESSED, &oldmask);

  while (1) {
    Card *deck;
    Pile *piles;
    int redeal;
    srand(seed);

    deck = new_deck();
    move_stack(deck, shuffle_stack(take_stack(deck->next)));

    piles = deal_cards(game, deck);
    deals++;

    redeal = ui_loop(game, theme, piles);
    delete_piles(piles);
    delete_stack(deck);

    if (redeal) {
      seed = time(NULL) + deals;
    } else {
      break;
    }
  }

  endwin();
}
