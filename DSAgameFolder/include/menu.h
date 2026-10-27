#ifndef MENU_H
#define MENU_H
#include <stdbool.h>
#include "input.h"

// Result of a main menu choice.
typedef enum {
    MENU_NONE,
    MENU_CONTINUE,      // Open level select.
    MENU_NEW_GAME,
    MENU_EDITOR,        // Open the editor.
    MENU_QUIT
} MenuAction;

// Special results from level select.
#define LEVELSEL_NONE (-1)
#define LEVELSEL_BACK (-2)
#define LEVELSEL_ADD  (-3)      // Add a level.

// Result while waiting for an answer.
#define CONFIRM_PENDING (-1)

// Draws the main menu and handles input.
MenuAction menu_main(Command cmd, bool can_continue);

// Sets the selected level when opening the screen.
void menu_level_select_reset(int cursor);

// Draws level choices and returns the selected result.
int menu_level_select(Command cmd, const char *title, int num_levels,
                      int unlocked, bool allow_add);

// Draws a Yes / No question and returns the answer.
int menu_confirm(Command cmd, const char *question);

// Draws the in-game level select button.
bool menu_levels_button(void);

#endif
