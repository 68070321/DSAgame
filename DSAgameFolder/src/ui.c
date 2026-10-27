#include "ui.h"
#include "raylib.h"

#define TILE 48
#define SCREEN_W 800
#define AREA_TOP 50
#define AREA_BOTTOM 470

void ui_draw_grid(const Level *lv, int ox, int oy, int tile) {
    int inset = tile / 8;
    int goal_r = tile / 8 < 3 ? 3 : tile / 8;

    for (int y = 0; y < lv->height; y++) {
        for (int x = 0; x < lv->width; x++) {
            char c = lv->grid[y][x];
            int px = ox + x * tile;
            int py = oy + y * tile;

            if (c == '#') {
                DrawRectangle(px, py, tile, tile, DARKGRAY);
                continue;
            }
            DrawRectangle(px, py, tile, tile, LIGHTGRAY);
            DrawRectangleLines(px, py, tile, tile, GRAY);

            if (c == '.' || c == '*' || c == '+')
                DrawCircle(px + tile / 2, py + tile / 2, goal_r, RED);
            if (c == '$' || c == '*')
                DrawRectangle(px + inset, py + inset, tile - 2 * inset, tile - 2 * inset,
                              c == '*' ? GREEN : BROWN);
            if (c == '@' || c == '+')
                DrawCircle(px + tile / 2, py + tile / 2, tile / 3, BLUE);
        }
    }
}

void ui_draw(const Level *lv) {
    int map_w = lv->width * TILE;
    int map_h = lv->height * TILE;

    int ox = (SCREEN_W - map_w) / 2;
    int oy = AREA_TOP + (AREA_BOTTOM - AREA_TOP - map_h) / 2;
    if (ox < 0) ox = 0;
    if (oy < AREA_TOP) oy = AREA_TOP;

    ui_draw_grid(lv, ox, oy, TILE);

    DrawText(TextFormat("Moves: %d", lv->moves), ox, oy - 30, 24, DARKGRAY);
    DrawText("Arrows/WASD: move   R: restart", ox, oy + map_h + 12, 20, GRAY);
}

bool ui_button(int x, int y, int w, int h, const char *label, bool focused, bool enabled) {
    Rectangle r = { (float)x, (float)y, (float)w, (float)h };
    bool hover = CheckCollisionPointRec(GetMousePosition(), r);

    Color fill = !enabled ? LIGHTGRAY : (focused ? SKYBLUE : (Color){235, 235, 235, 255});
    DrawRectangleRec(r, fill);
    DrawRectangleLinesEx(r, 2, (focused && enabled) ? BLUE : GRAY);

    int size = h * 5 / 9;
    if (size > 24) size = 24;
    int tw = MeasureText(label, size);
    DrawText(label, x + (w - tw) / 2, y + (h - size) / 2, size, enabled ? DARKGRAY : GRAY);

    return enabled && hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
