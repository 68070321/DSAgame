#ifndef SAVE_H
#define SAVE_H
#include <stdbool.h>

typedef struct {
    int unlocked;   // Highest level the player can play.
    int last;       // Last level the player opened.
} SaveData;

// Loads saved progress. Missing files start at level 0.
SaveData save_load(const char *path, int num_levels);

// Saves progress and returns false if saving fails.
bool save_write(const char *path, const SaveData *data);

#endif
