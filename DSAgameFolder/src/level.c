#include "level.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

bool level_load(Level *lv, const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return false;

    memset(lv->grid, ' ', sizeof(lv->grid));
    lv->width = lv->height = lv->moves = 0;
    lv->player_x = lv->player_y = -1;

    char line[256];
    while (lv->height < MAX_H && fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = '\0';
        int len = (int)strlen(line);
        if (len > MAX_W) len = MAX_W;
        for (int x = 0; x < len; x++) {
            lv->grid[lv->height][x] = line[x];
            if (line[x] == '@' || line[x] == '+') {
                lv->player_x = x;
                lv->player_y = lv->height;
            }
        }
        if (len > lv->width) lv->width = len;
        lv->height++;
    }
    fclose(f);
    return lv->height > 0 && lv->player_x >= 0;    // A level needs a player.
}

bool level_save(const Level *lv, const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return false;

    bool ok = true;
    for (int y = 0; y < lv->height; y++) {
        // Write each row at full width to keep the map size.
        if (fwrite(lv->grid[y], 1, (size_t)lv->width, f) != (size_t)lv->width) ok = false;
        if (fputc('\n', f) == EOF) ok = false;
    }
    if (fclose(f) != 0) ok = false;
    return ok;
}

void level_path(int index, char *out, size_t size) {
    snprintf(out, size, "levels/level%02d.txt", index + 1);
}

int level_count(void) {
    int n = 0;
    char path[64];
    while (n < MAX_LEVELS) {
        level_path(n, path, sizeof(path));
        FILE *f = fopen(path, "r");
        if (!f) break;
        fclose(f);
        n++;
    }
    return n;
}
