#ifndef LEVEL_H
#define LEVEL_H
#include <stdbool.h>
#include <stddef.h>

#define MAX_W 20
#define MAX_H 20
#define MAX_LEVELS 25   // Maximum number of levels

typedef struct {
    char grid[MAX_H][MAX_W];  // Map symbols show walls, goals, boxes and the player.
    int width, height;
    int player_x, player_y;
    int moves;
} Level;

bool level_load(Level *lv, const char *path);

// Saves a level to a file.
bool level_save(const Level *lv, const char *path);

// Builds the file name for a level.
void level_path(int index, char *out, size_t size);

// Counts the level files.
int level_count(void);

#endif
