#ifndef GAME_H
#define GAME_H
#include "level.h"

// Moves the player and returns false if the move is blocked.
bool game_move(Level *lv, int dx, int dy);
bool game_is_won(const Level *lv);

#endif
