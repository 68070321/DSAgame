#ifndef UI_H
#define UI_H
#include <stdbool.h>
#include "level.h"

// Draws the level and move count.
void ui_draw(const Level *lv);

// Draws the grid using the given tile size.
void ui_draw_grid(const Level *lv, int ox, int oy, int tile);

// Draws a button and returns true when clicked.
bool ui_button(int x, int y, int w, int h, const char *label, bool focused, bool enabled);

#endif
