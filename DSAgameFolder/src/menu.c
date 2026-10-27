#include "menu.h"
#include "ui.h"
#include "raylib.h"

#define MAIN_ITEMS 4

#define COLS     5
#define CELL_W   100
#define CELL_H   70
#define GAP      20
#define GRID_TOP 110

static int main_sel = 0;
static int level_sel = 0;
static int confirm_sel = 0;

static bool button(Rectangle r, const char *label, bool focused, bool enabled) {
    return ui_button((int)r.x, (int)r.y, (int)r.width, (int)r.height,
                     label, focused, enabled);
}

// Only move focus with the mouse when it moves.
static bool mouse_moved(void) {
    Vector2 d = GetMouseDelta();
    return d.x != 0 || d.y != 0;
}

static void draw_hint(const char *text) {
    DrawText(text, (GetScreenWidth() - MeasureText(text, 18)) / 2,
             GetScreenHeight() - 40, 18, GRAY);
}

MenuAction menu_main(Command cmd, bool can_continue) {
    const char *labels[MAIN_ITEMS] = { "Continue", "New Game", "Level Editor", "Quit" };
    const MenuAction actions[MAIN_ITEMS] = { MENU_CONTINUE, MENU_NEW_GAME, MENU_EDITOR, MENU_QUIT };
    const bool enabled[MAIN_ITEMS] = { can_continue, true, true, true };
    int w = GetScreenWidth();

    // Skip disabled menu items.
    if (!enabled[main_sel]) main_sel = 1;

    int step = 0;
    if (cmd == CMD_UP)   step = MAIN_ITEMS - 1;
    if (cmd == CMD_DOWN) step = 1;
    if (step != 0) {
        do {
            main_sel = (main_sel + step) % MAIN_ITEMS;
        } while (!enabled[main_sel]);
    }

    const char *title = "Sokoban Studio";
    DrawText(title, (w - MeasureText(title, 56)) / 2, 100, 56, DARKGRAY);

    MenuAction result = MENU_NONE;
    for (int i = 0; i < MAIN_ITEMS; i++) {
        Rectangle r = { (float)(w - 240) / 2, (float)(220 + i * 72), 240, 56 };
        if (enabled[i] && mouse_moved() && CheckCollisionPointRec(GetMousePosition(), r))
            main_sel = i;
        if (button(r, labels[i], main_sel == i, enabled[i]))
            result = actions[i];
    }

    if (cmd == CMD_CONFIRM) result = actions[main_sel];

    draw_hint("2/8 or Up/Down: choose     # or Enter: select");
    return result;
}

void menu_level_select_reset(int cursor) {
    level_sel = cursor;
}

int menu_level_select(Command cmd, const char *title, int num_levels,
                      int unlocked, bool allow_add) {
    int w = GetScreenWidth();

    int last_open = unlocked;
    if (last_open > num_levels - 1) last_open = num_levels - 1;
    int max_sel = allow_add ? num_levels : last_open;
    if (max_sel < 0) max_sel = 0;
    if (level_sel > max_sel) level_sel = max_sel;
    if (level_sel < 0) level_sel = 0;

    // The cursor can only select unlocked levels.
    int next = level_sel;
    if (cmd == CMD_LEFT)  next = level_sel - 1;
    if (cmd == CMD_RIGHT) next = level_sel + 1;
    if (cmd == CMD_UP)    next = level_sel - COLS;
    if (cmd == CMD_DOWN)  next = level_sel + COLS;
    if (next >= 0 && next <= max_sel) level_sel = next;

    DrawText(title, (w - MeasureText(title, 36)) / 2, 30, 36, DARKGRAY);

    int grid_w = COLS * CELL_W + (COLS - 1) * GAP;
    int x0 = (w - grid_w) / 2;
    int cells = allow_add ? num_levels + 1 : num_levels;

    int chosen = LEVELSEL_NONE;
    for (int i = 0; i < cells; i++) {
        int col = i % COLS;
        int row = i / COLS;
        Rectangle r = { (float)(x0 + col * (CELL_W + GAP)),
                        (float)(GRID_TOP + row * (CELL_H + GAP)),
                        CELL_W, CELL_H };
        bool is_add = allow_add && i == num_levels;
        bool open = is_add || i <= last_open;

        if (open && mouse_moved() && CheckCollisionPointRec(GetMousePosition(), r))
            level_sel = i;

        const char *label = is_add ? "+" : (open ? TextFormat("%d", i + 1) : "Locked");
        if (button(r, label, open && level_sel == i, open))
            chosen = is_add ? LEVELSEL_ADD : i;
    }

    Rectangle back = { 20, 20, 180, 44 };
    if (button(back, "Back to menu", false, true)) chosen = LEVELSEL_BACK;

    if (cmd == CMD_CONFIRM && (allow_add || num_levels > 0))
        chosen = (allow_add && level_sel == num_levels) ? LEVELSEL_ADD : level_sel;
    if (cmd == CMD_BACK) chosen = LEVELSEL_BACK;

    draw_hint("2/4/6/8 or arrows: move     # or Enter: select     0 or Esc: back");
    return chosen;
}

int menu_confirm(Command cmd, const char *question) {
    int w = GetScreenWidth();
    const char *labels[2] = { "No", "Yes" };

    if (cmd == CMD_LEFT || cmd == CMD_RIGHT || cmd == CMD_UP || cmd == CMD_DOWN)
        confirm_sel = 1 - confirm_sel;

    DrawText(question, (w - MeasureText(question, 28)) / 2, 200, 28, DARKGRAY);

    int answer = CONFIRM_PENDING;
    for (int i = 0; i < 2; i++) {
        Rectangle r = { (float)(w / 2 - 170 + i * 190), 290, 150, 56 };
        if (mouse_moved() && CheckCollisionPointRec(GetMousePosition(), r))
            confirm_sel = i;
        if (button(r, labels[i], confirm_sel == i, true))
            answer = i;
    }

    if (cmd == CMD_CONFIRM) answer = confirm_sel;
    if (cmd == CMD_BACK)    answer = 0;
    if (answer != CONFIRM_PENDING) confirm_sel = 0;

    draw_hint("4/6 or arrows: choose     # or Enter: confirm     0 or Esc: no");
    return answer;
}

bool menu_levels_button(void) {
    Rectangle r = { 10, 6, 120, 36 };
    return button(r, "Levels", false, true);
}
