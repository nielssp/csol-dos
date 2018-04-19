/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "config.h"
#include "rc.h"
#include "theme.h"
#include "game.h"
#include "ui.h"
#include "util.h"

const char *short_options = "hvlt:Tms:c:";

enum action { PLAY, LIST_GAMES, LIST_THEMES };

void describe_option(const char *short_option, const char *long_option, const char *description) {
  printf("  -%-14s --%-18s %s\n", short_option, long_option, description);
}

char *find_csolrc() {
  FILE *f = fopen("csolrc", "r");
  if (f) {
    fclose(f);
    return strdup("csolrc");
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  int opt, rc_opt, error;
  int colors = 1;
  unsigned int seed = time(NULL);
  enum action action = PLAY;
  char *rc_file = NULL;
  char *game_name = NULL;
  char *theme_name = NULL;
  Theme *theme;
  Game *game;
  while ((opt = getopt(argc, argv, short_options)) != -1) {
    switch (opt) {
      case 'h':
        printf("usage: %s [options] [game]\n", argv[0]);
        puts("options:");
        describe_option("h", "help", "Show help.");
        describe_option("v", "version", "Show version information.");
        describe_option("l", "list", "List available games.");
        describe_option("t <name>", "theme <name>", "Select a theme.");
        describe_option("T", "themes", "List available themes.");
        describe_option("m", "mono", "Disable colors.");
        describe_option("s <seed>", "seed <seed>", "Select seed.");
        describe_option("c <file>", "config <file>", "Select configuration file.");
        return 0;
      case 'v':
        puts("csol " CSOL_VERSION);
        return 0;
      case 'l':
        action = LIST_GAMES;
        break;
      case 't':
        theme_name = optarg;
        break;
      case 'T':
        action = LIST_THEMES;
        break;
      case 'm':
        colors = 0;
        break;
      case 's':
        seed = atol(optarg);
        break;
      case 'c':
        rc_file = optarg;
        break;

    }
  }
  if (optind < argc) {
    game_name = argv[optind];
  }
  rc_opt = 1;
  error = 0;
  if (!rc_file) {
    rc_opt = 0;
    rc_file = find_csolrc();
    if (!rc_file) {
      printf("csolrc: %s\n", strerror(errno));
      error = 1;
    }
  }
  if (!error) {
    printf("Using configuration file: %s\n", rc_file);
    error = !execute_file(rc_file);
    if (!rc_opt) {
      free(rc_file);
    }
  }
  if (error) {
    printf("Configuration errors detected, press enter to continue\n");
    getchar();
  }
  switch (action) {
    case LIST_GAMES: {
      GameList *list;
      load_game_dirs();
      for (list = list_games(); list; list = list->next) {
        printf("%s - %s\n", list->game->name, list->game->title);
      }
      break;
    }
    case LIST_THEMES: {
      ThemeList *list;
      load_theme_dirs();
      for (list = list_themes(); list; list = list->next) {
        printf("%s - %s\n", list->theme->name, list->theme->title);
      }
      break;
    }
    case PLAY:
      if (theme_name == NULL) {
        theme_name = get_property("default_theme");
        if (theme_name == NULL) {
          printf("default_theme not set\n");
          return 1;
        }
      }
      theme = get_theme(theme_name);
      if (!theme) {
        printf("theme not found: '%s'\n", theme_name);
        return 1;
      }
      if (game_name == NULL) {
        game_name = get_property("default_game");
        if (game_name == NULL) {
          printf("default_game not set\n");
          return 1;
        }
      }
      game = get_game(game_name);
      if (!game) {
        printf("game not found: '%s'\n", game_name);
        return 1;
      }
      ui_main(game, theme, colors, seed);
      break;
  }
  return 0;
}
