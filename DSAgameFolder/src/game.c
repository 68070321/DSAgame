#include "game.h"

static bool is_box(char c)  { return c == '$' || c == '*'; }
static bool is_goal(char c) { return c == '.' || c == '*' || c == '+'; }
static bool in_bounds(const Level *lv, int x, int y) {
    return x >= 0 && y >= 0 && x < lv->width && y < lv->height;
}

bool game_move(Level *lv, int dx, int dy) {
    int px = lv->player_x, py = lv->player_y;
    int nx = px + dx, ny = py + dy;
    if (!in_bounds(lv, nx, ny)) return false;

    char target = lv->grid[ny][nx];
    if (target == '#') return false;

    if (is_box(target)) {
        int bx = nx + dx, by = ny + dy;
        if (!in_bounds(lv, bx, by)) return false;
        char beyond = lv->grid[by][bx];
        if (beyond == '#' || is_box(beyond)) return false;

        lv->grid[by][bx] = is_goal(beyond) ? '*' : '$';
        lv->grid[ny][nx] = is_goal(target) ? '.' : ' ';
    }

    lv->grid[py][px] = (lv->grid[py][px] == '+') ? '.' : ' ';
    lv->grid[ny][nx] = is_goal(lv->grid[ny][nx]) ? '+' : '@';
    lv->player_x = nx;
    lv->player_y = ny;
    lv->moves++;
    return true;
}

bool game_is_won(const Level *lv) {
    for (int y = 0; y < lv->height; y++)
        for (int x = 0; x < lv->width; x++)
            if (lv->grid[y][x] == '$') return false;
    return true;
}
