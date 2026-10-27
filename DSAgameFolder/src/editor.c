#include "editor.h"
#include "ui.h"
#include "solver.h"
#include "raylib.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define NEW_W    10         // size of a brand new level
#define NEW_H    8
#define MIN_SIZE 3

// Where the map is drawn (left part of the window) and where the tool panel is
#define GRID_X 20
#define GRID_Y 56
#define GRID_W 580
#define GRID_H 410
#define PANEL_X 620
#define PANEL_W 160

// A cell is a stack of layers. Splitting the map characters into layers makes
// the tools easy: a goal and a box can sit on the same cell, and so can a goal
// and the player.
typedef struct {
    bool wall, goal, box, player;
} Cell;

static Cell cell_decode(char c) {
    Cell k;
    k.wall   = (c == '#');
    k.goal   = (c == '.' || c == '*' || c == '+');
    k.box    = (c == '$' || c == '*');
    k.player = (c == '@' || c == '+');
    return k;
}

static char cell_encode(Cell k) {
    if (k.wall)   return '#';
    if (k.player) return k.goal ? '+' : '@';
    if (k.box)    return k.goal ? '*' : '$';
    return k.goal ? '.' : ' ';
}

static void set_status(Editor *ed, bool error, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(ed->status, sizeof(ed->status), fmt, args);
    va_end(args);
    ed->status_error = error;
}

// Walls all around, empty floor inside, the player in the top-left corner.
static void blank_level(Level *lv) {
    memset(lv->grid, ' ', sizeof(lv->grid));
    lv->width = NEW_W;
    lv->height = NEW_H;
    lv->moves = 0;
    for (int y = 0; y < NEW_H; y++)
        for (int x = 0; x < NEW_W; x++)
            if (x == 0 || y == 0 || x == NEW_W - 1 || y == NEW_H - 1)
                lv->grid[y][x] = '#';
    lv->grid[1][1] = '@';
    lv->player_x = 1;
    lv->player_y = 1;
}

void editor_new(Editor *ed, int index) {
    memset(ed, 0, sizeof(*ed));
    ed->index = index;
    ed->is_new = true;
    blank_level(&ed->lv);
    ed->cx = 1;
    ed->cy = 1;
    ed->brush = TOOL_WALL;
    set_status(ed, false, "New level. Build it, then press # to save");
}

void editor_edit(Editor *ed, int index) {
    memset(ed, 0, sizeof(*ed));
    ed->index = index;
    ed->brush = TOOL_WALL;

    char path[64];
    level_path(index, path, sizeof(path));
    if (level_load(&ed->lv, path)) {
        ed->cx = ed->lv.player_x;
        ed->cy = ed->lv.player_y;
    } else {
        blank_level(&ed->lv);
        ed->cx = 1;
        ed->cy = 1;
        set_status(ed, true, "Could not read %s, started a blank level", path);
    }
}

// Applies a tool to the cell under the cursor.
static void apply_tool(Editor *ed, Tool tool) {
    Level *lv = &ed->lv;
    char *cell = &lv->grid[ed->cy][ed->cx];
    Cell k = cell_decode(*cell);

    ed->brush = tool;
    ed->back_armed = false;

    switch (tool) {
        case TOOL_WALL:
            if (k.player) { set_status(ed, true, "The start can't be a wall. Move the start first"); return; }
            if (k.wall) {
                k.wall = false;
            } else {
                k.wall = true;
                k.box = false;
                k.goal = false;
            }
            break;

        case TOOL_BOX:
            if (k.wall || k.player) { set_status(ed, true, "A box can't go there"); return; }
            k.box = !k.box;
            break;

        case TOOL_GOAL:
            if (k.wall) { set_status(ed, true, "A goal can't go on a wall"); return; }
            k.goal = !k.goal;
            break;

        case TOOL_START:
            if (k.wall || k.box) { set_status(ed, true, "The start can't go there"); return; }
            if (k.player) return;                       // already the start
            {
                // take the start away from its old cell
                char *old = &lv->grid[lv->player_y][lv->player_x];
                Cell o = cell_decode(*old);
                o.player = false;
                *old = cell_encode(o);
            }
            k.player = true;
            lv->player_x = ed->cx;
            lv->player_y = ed->cy;
            break;

        case TOOL_ERASE:
            if (k.wall) {
                k.wall = false;
            } else {
                k.box = false;
                k.goal = false;                         // the start stays
            }
            break;
    }

    *cell = cell_encode(k);
    ed->dirty = true;
    set_status(ed, false, "");
}

static void resize(Editor *ed, int dw, int dh) {
    Level *lv = &ed->lv;
    int nw = lv->width + dw;
    int nh = lv->height + dh;

    if (nw < MIN_SIZE || nh < MIN_SIZE || nw > MAX_W || nh > MAX_H) {
        set_status(ed, true, "Width and height must be between %d and %d", MIN_SIZE, MAX_W);
        return;
    }
    if (lv->player_x >= nw || lv->player_y >= nh) {
        set_status(ed, true, "Move the start (7) inside the new size first");
        return;
    }

    // keep everything outside the map empty, so growing again gives clean floor
    for (int y = 0; y < MAX_H; y++)
        for (int x = 0; x < MAX_W; x++)
            if (x >= nw || y >= nh) lv->grid[y][x] = ' ';

    lv->width = nw;
    lv->height = nh;
    if (ed->cx >= nw) ed->cx = nw - 1;
    if (ed->cy >= nh) ed->cy = nh - 1;
    ed->dirty = true;
    ed->back_armed = false;
    set_status(ed, false, "Size: %d x %d", nw, nh);
}

// Basic rules a level must follow. Returns true and fills `msg` if something is wrong.
static bool level_problem(const Level *lv, char *msg, size_t size) {
    int boxes = 0, goals = 0, unsolved = 0;

    for (int y = 0; y < lv->height; y++) {
        for (int x = 0; x < lv->width; x++) {
            char c = lv->grid[y][x];
            if (c == '$' || c == '*') boxes++;
            if (c == '.' || c == '*' || c == '+') goals++;
            if (c == '$') unsolved++;
        }
    }

    if (boxes == 0)
        snprintf(msg, size, "add at least one box (3)");
    else if (goals != boxes)
        snprintf(msg, size, "%d boxes but %d goals. They must match", boxes, goals);
    else if (unsolved == 0)
        snprintf(msg, size, "every box is already on a goal");
    else
        return false;
    return true;
}

// Runs the solver on the level as it is now. Shows the result in the status bar
// and returns it. The search can take a moment on big or open maps.
static SolveStatus check_level(Editor *ed, bool announce) {
    char problem[64];
    if (level_problem(&ed->lv, problem, sizeof(problem))) {
        set_status(ed, true, "Can't check: %s", problem);
        return SOLVE_ERROR;
    }

    Solution sol = solver_solve(&ed->lv);
    SolveStatus status = sol.status;
    int moves = sol.count;
    solver_free_solution(&sol);

    if (announce) {
        switch (status) {
            case SOLVE_FOUND:
                set_status(ed, false, "Solvable! Best solution: %d moves", moves);
                break;
            case SOLVE_UNWINNABLE:
                set_status(ed, true, "UNWINNABLE: this level can't be completed");
                break;
            case SOLVE_TOO_BIG:
                set_status(ed, true, "Too complex to check. It might still be winnable");
                break;
            default:
                set_status(ed, true, "Check failed: out of memory");
                break;
        }
    }
    return status;
}

// Checks that the level follows the rules, then writes the file. A level that
// can't be won is still saved (it may be half finished) but with a loud warning.
static void save_level(Editor *ed) {
    char problem[64];
    ed->back_armed = false;

    if (level_problem(&ed->lv, problem, sizeof(problem))) {
        set_status(ed, true, "Can't save: %s", problem);
        return;
    }

    char path[64];
    level_path(ed->index, path, sizeof(path));
    if (!level_save(&ed->lv, path)) {
        set_status(ed, true, "Could not write %s", path);
        return;
    }
    ed->dirty = false;
    ed->is_new = false;

    switch (check_level(ed, false)) {
        case SOLVE_FOUND:
            set_status(ed, false, "Saved %s. Solvable", path);
            break;
        case SOLVE_UNWINNABLE:
            set_status(ed, true, "Saved, but WARNING: this level can't be won!");
            break;
        case SOLVE_TOO_BIG:
            set_status(ed, false, "Saved %s. Too complex to check", path);
            break;
        default:
            set_status(ed, false, "Saved %s", path);
            break;
    }
}

// Returns true if it is fine to leave. With unsaved changes the first try only
// shows a warning, and a second try discards them.
static bool try_leave(Editor *ed) {
    if (!ed->dirty || ed->back_armed) return true;
    ed->back_armed = true;
    set_status(ed, true, "Unsaved changes! Press back again to discard, or # to save");
    return false;
}

EditorResult editor_run(Editor *ed, Command cmd) {
    Level *lv = &ed->lv;
    EditorResult result = EDITOR_STAY;

    // ---------- check key (read here so no other file needs a new command) ----------
    if (IsKeyPressed(KEY_T)) check_level(ed, true);

    // ---------- keypad / keyboard ----------
    switch (cmd) {
        case CMD_UP:       if (ed->cy > 0) ed->cy--; break;
        case CMD_DOWN:     if (ed->cy < lv->height - 1) ed->cy++; break;
        case CMD_LEFT:     if (ed->cx > 0) ed->cx--; break;
        case CMD_RIGHT:    if (ed->cx < lv->width - 1) ed->cx++; break;
        case CMD_WALL:     apply_tool(ed, TOOL_WALL);  break;
        case CMD_BOX:      apply_tool(ed, TOOL_BOX);   break;
        case CMD_GOAL:     apply_tool(ed, TOOL_GOAL);  break;
        case CMD_START:    apply_tool(ed, TOOL_START); break;
        case CMD_ERASE:    apply_tool(ed, TOOL_ERASE); break;
        case CMD_NARROWER: resize(ed, -1, 0); break;
        case CMD_WIDER:    resize(ed, 1, 0);  break;
        case CMD_SHORTER:  resize(ed, 0, -1); break;
        case CMD_TALLER:   resize(ed, 0, 1);  break;
        case CMD_SAVE:     save_level(ed); break;
        case CMD_BACK:     if (try_leave(ed)) result = EDITOR_EXIT; break;
        case CMD_CHECK: check_level(ed, true); break;
        default: break;
    }

    // ---------- map position (shrinks the tiles for big maps) ----------
    int tile = GRID_W / lv->width;
    int tile_h = GRID_H / lv->height;
    if (tile_h < tile) tile = tile_h;
    if (tile > 48) tile = 48;
    int ox = GRID_X + (GRID_W - tile * lv->width) / 2;
    int oy = GRID_Y + (GRID_H - tile * lv->height) / 2;

    // ---------- mouse: click a cell to apply the selected tool ----------
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 m = GetMousePosition();
        if (m.x >= ox && m.y >= oy &&
            m.x < ox + tile * lv->width && m.y < oy + tile * lv->height) {
            ed->cx = ((int)m.x - ox) / tile;
            ed->cy = ((int)m.y - oy) / tile;
            apply_tool(ed, ed->brush);
        }
    }

    // ---------- draw the map and the cursor ----------
    DrawText(TextFormat("Level Editor - level %d%s%s", ed->index + 1,
                        ed->is_new ? " (new)" : "", ed->dirty ? " *" : ""),
             20, 14, 26, DARKGRAY);

    ui_draw_grid(lv, ox, oy, tile);
    DrawRectangleLinesEx((Rectangle){ (float)(ox + ed->cx * tile), (float)(oy + ed->cy * tile),
                                      (float)tile, (float)tile }, 3, ORANGE);

    // ---------- tool panel ----------
    static const char *tool_names[5] = {
        "Wall (1)", "Box (3)", "Goal (5)", "Start (7)", "Erase (9)"
    };
    for (int i = 0; i < 5; i++) {
        if (ui_button(PANEL_X, 56 + i * 44, PANEL_W, 38, tool_names[i], ed->brush == (Tool)i, true))
            ed->brush = (Tool)i;
    }

    DrawText(TextFormat("Size: %d x %d", lv->width, lv->height), PANEL_X, 284, 20, DARKGRAY);
    const char *size_labels[4] = { "W-", "W+", "H-", "H+" };
    const int size_dw[4] = { -1, 1, 0, 0 };
    const int size_dh[4] = { 0, 0, -1, 1 };
    for (int i = 0; i < 4; i++) {
        if (ui_button(PANEL_X + i * 40, 312, 35, 32, size_labels[i], false, true))
            resize(ed, size_dw[i], size_dh[i]);
    }

    if (ui_button(PANEL_X, 352, PANEL_W, 36, "Check (T)", false, true))
        check_level(ed, true);
    if (ui_button(PANEL_X, 394, PANEL_W, 36, "Save (#)", false, true))
        save_level(ed);
    if (ui_button(PANEL_X, 436, PANEL_W, 36, "Back (0)", false, true))
        if (try_leave(ed)) result = EDITOR_EXIT;

    // ---------- hints and status ----------
    DrawText("Keypad: 2/4/6/8 move  1 wall  3 box  5 goal  7 start  9 erase", 20, 482, 18, GRAY);
    DrawText("A/B width -/+  C/D height -/+  # save  0 back  (keyboard: Z/X, C/V, Enter)",
             20, 504, 18, GRAY);
    DrawText("Mouse: pick a tool, click a cell.   T or *: is it winnable?", 20, 526, 18, GRAY);
    if (ed->status[0] != '\0')
        DrawText(ed->status, 20, 556, 22, ed->status_error ? RED : DARKGREEN);

    return result;
}
