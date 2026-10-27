#include "raylib.h"
#include "level.h"
#include "game.h"
#include "ui.h"
#include "stack.h"
#include "save.h"
#include "input.h"
#include "serial.h"
#include "menu.h"
#include "editor.h"
#include "solver.h"

#include <stdio.h>
#include <string.h>

#define BAUD_RATE 9600          // Must match the Arduino baud rate.

// Screens shown by the game.
typedef enum {
    SCREEN_MENU,
    SCREEN_LEVELS,
    SCREEN_PLAY,
    SCREEN_CONFIRM_NEW,
    SCREEN_EDIT_SELECT,
    SCREEN_EDIT
} Screen;

static SaveData g_save;
static char g_save_path[512];
static bool g_save_ok = true;

static void persist(void) {
    g_save_ok = save_write(g_save_path, &g_save);
}

// Continue is available after the player makes progress.
static bool has_progress(void) {
    return g_save.unlocked > 0 || g_save.last > 0;
}

// Loads a level and clears undo and redo history.
static bool load_level(Level *lv, int index, Stack *undo, Stack *redo) {
    char path[64];
    stack_clear(undo);
    stack_clear(redo);
    level_path(index, path, sizeof(path));
    return level_load(lv, path);
}

// Saves the selected level as the last played level, then loads it.
static bool start_level(Level *lv, int index, Stack *undo, Stack *redo) {
    g_save.last = index;
    persist();
    return load_level(lv, index, undo, redo);
}

// Clears progress and starts the first level.
static bool new_game(Level *lv, Stack *undo, Stack *redo) {
    g_save.unlocked = 0;
    return start_level(lv, 0, undo, redo);
}

// Reads the serial port from port.txt, or uses COM3 if the file is missing.
static void read_port_name(char *out, size_t size) {
    snprintf(out, size, "COM3");
    FILE *f = fopen("port.txt", "r");
    if (!f) return;

    char buf[32];
    if (fgets(buf, sizeof(buf), f)) {
        char *p = buf + strspn(buf, " \t");
        p[strcspn(p, " \t\r\n")] = '\0';
        if (p[0] != '\0') snprintf(out, size, "%s", p);
    }
    fclose(f);
}

// Keeps save.txt next to the game, wherever it was started from.
static void build_save_path(char *out, size_t size) {
    const char *dir = GetApplicationDirectory();
    size_t n = strlen(dir);
    const char *sep = (n > 0 && (dir[n - 1] == '/' || dir[n - 1] == '\\')) ? "" : "/";
    snprintf(out, size, "%s%ssave.txt", dir, sep);
}

// Uses the editor's key layout while the editor is open.
static Command poll_command(Screen screen) {
    int c;
    while ((c = serial_read_key()) != -1) {
        Command cmd = (screen == SCREEN_EDIT) ? input_from_key_editor(c)
                                              : input_from_key(c);
        if (cmd != CMD_NONE) return cmd;
    }

    if (screen == SCREEN_EDIT) {
        if (IsKeyPressed(KEY_ONE)   || IsKeyPressed(KEY_KP_1)) return CMD_WALL;
        if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) return CMD_BOX;
        if (IsKeyPressed(KEY_FIVE)  || IsKeyPressed(KEY_KP_5)) return CMD_GOAL;
        if (IsKeyPressed(KEY_SEVEN) || IsKeyPressed(KEY_KP_7)) return CMD_START;
        if (IsKeyPressed(KEY_NINE)  || IsKeyPressed(KEY_KP_9)) return CMD_ERASE;
        if (IsKeyPressed(KEY_Z)) return CMD_NARROWER;
        if (IsKeyPressed(KEY_X)) return CMD_WIDER;
        if (IsKeyPressed(KEY_C)) return CMD_SHORTER;
        if (IsKeyPressed(KEY_V)) return CMD_TALLER;
        if (IsKeyPressed(KEY_H)) return CMD_SOLVE;
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) return CMD_SAVE;
    }

    if (IsKeyPressed(KEY_UP)    || IsKeyPressed(KEY_W)) return CMD_UP;
    if (IsKeyPressed(KEY_DOWN)  || IsKeyPressed(KEY_S)) return CMD_DOWN;
    if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) return CMD_LEFT;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) return CMD_RIGHT;
    if (IsKeyPressed(KEY_U))      return CMD_UNDO;
    if (IsKeyPressed(KEY_Y))      return CMD_REDO;
    if (IsKeyPressed(KEY_R))      return CMD_RESTART;
    if (IsKeyPressed(KEY_ENTER))  return CMD_CONFIRM;
    if (IsKeyPressed(KEY_P))      return CMD_PREV;
    if (IsKeyPressed(KEY_N))      return CMD_NEXT;
    if (IsKeyPressed(KEY_L))      return CMD_LEVELS;
    if (IsKeyPressed(KEY_ESCAPE)) return CMD_BACK;
    if (IsKeyPressed(KEY_H))      return CMD_SOLVE;
    return CMD_NONE;
}

int main(void) {
    const char *hint_msg = "";
    InitWindow(800, 600, "Sokoban Studio");
    SetExitKey(KEY_NULL);               // Escape goes back instead of closing.
    SetTargetFPS(60);

    build_save_path(g_save_path, sizeof(g_save_path));
    SetWindowTitle(TextFormat("Sokoban Studio - save file: %s", g_save_path));

    char port[32];
    read_port_name(port, sizeof(port));
    bool arduino = serial_open(port, BAUD_RATE);

    Stack undo, redo;
    stack_init(&undo);
    stack_init(&redo);

    int num_levels = level_count();
    g_save = save_load(g_save_path, num_levels > 0 ? num_levels : 1);
    int current = g_save.last;
    Level lv;
    bool loaded = false;
    Editor ed;

    Screen screen = SCREEN_MENU;
    bool quit = false;

    while (!quit && !WindowShouldClose()) {
        Command cmd = poll_command(screen);
        bool won = false;
        bool go_levels = false;

        if (screen == SCREEN_PLAY) {
            if (cmd == CMD_LEVELS || cmd == CMD_BACK) go_levels = true;

            if (loaded) {
                won = game_is_won(&lv);

                if (!won) {
                    int dx = 0, dy = 0;
                    if (cmd == CMD_UP)    dy = -1;
                    if (cmd == CMD_DOWN)  dy = 1;
                    if (cmd == CMD_LEFT)  dx = -1;
                    if (cmd == CMD_RIGHT) dx = 1;
                                        if (cmd == CMD_SOLVE) {
                        Solution sol = solver_solve(&lv);
                        switch (sol.status) {
                            case SOLVE_FOUND:
                                hint_msg = "";
                                if (sol.count > 0) {
                                    switch (sol.moves[0]) {
                                        case CMD_UP:    dy = -1; break;
                                        case CMD_DOWN:  dy =  1; break;
                                        case CMD_LEFT:  dx = -1; break;
                                        case CMD_RIGHT: dx =  1; break;
                                        default: break;
                                    }
                                }
                                break;
                            case SOLVE_UNWINNABLE:
                                hint_msg = "This level can't be won";
                                break;
                            case SOLVE_TOO_BIG:
                                hint_msg = "Too complex to check. It might still be winnable.";
                                break;
                            default:
                                hint_msg = "Solver ran out of memory.";
                                break;
                        }
                        solver_free_solution(&sol);
                    }

                    if (dx != 0 || dy != 0) {
                        Level before = lv;
                        if (game_move(&lv, dx, dy)) {
                            stack_push(&undo, &before);
                            stack_clear(&redo);
                        }
                    }
                }

                if (cmd == CMD_UNDO) {
                    Level prev;
                    if (stack_pop(&undo, &prev)) { stack_push(&redo, &lv); lv = prev; }
                }
                
                if (cmd == CMD_REDO) {
                    Level next;
                    if (stack_pop(&redo, &next)) { stack_push(&undo, &lv); lv = next; }
                }
                if (cmd == CMD_RESTART)
                    loaded = load_level(&lv, current, &undo, &redo);

                if (won && cmd == CMD_CONFIRM && current + 1 < num_levels) {
                    current++;
                    loaded = start_level(&lv, current, &undo, &redo);
                }

                if (cmd == CMD_PREV && current > 0) {
                    current--;
                    loaded = start_level(&lv, current, &undo, &redo);
                }
                if (cmd == CMD_NEXT && current < g_save.unlocked) {
                    current++;
                    loaded = start_level(&lv, current, &undo, &redo);
                }
            }

            won = loaded && game_is_won(&lv);
        }

        if (won && current + 1 < num_levels && g_save.unlocked < current + 1) {
            g_save.unlocked = current + 1;
            persist();
        } else if (!g_save_ok) {
            persist();
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (screen == SCREEN_MENU) {
            MenuAction action = menu_main(cmd, has_progress());
            if (action == MENU_CONTINUE) {
                menu_level_select_reset(g_save.last);
                screen = SCREEN_LEVELS;
            }
            if (action == MENU_NEW_GAME) {
                if (has_progress()) {
                    screen = SCREEN_CONFIRM_NEW;
                } else {
                    current = 0;
                    loaded = new_game(&lv, &undo, &redo);
                    screen = SCREEN_PLAY;
                }
            }
            if (action == MENU_EDITOR) {
                menu_level_select_reset(0);
                screen = SCREEN_EDIT_SELECT;
            }
            if (action == MENU_QUIT) quit = true;

        } else if (screen == SCREEN_CONFIRM_NEW) {
            int answer = menu_confirm(cmd, "Start over? Your saved progress will be erased.");
            if (answer == 1) {
                current = 0;
                loaded = new_game(&lv, &undo, &redo);
                screen = SCREEN_PLAY;
            } else if (answer == 0) {
                screen = SCREEN_MENU;
            }

        } else if (screen == SCREEN_LEVELS) {
            int pick = menu_level_select(cmd, "Select Level", num_levels,
                                         g_save.unlocked, false);
            if (pick == LEVELSEL_BACK) {
                screen = SCREEN_MENU;
            } else if (pick >= 0) {
                current = pick;
                loaded = start_level(&lv, current, &undo, &redo);
                screen = SCREEN_PLAY;
            }

        } else if (screen == SCREEN_EDIT_SELECT) {
            int pick = menu_level_select(cmd, "Level Editor", num_levels,
                                         num_levels - 1, num_levels < MAX_LEVELS);
            if (pick == LEVELSEL_BACK) {
                screen = SCREEN_MENU;
            } else if (pick == LEVELSEL_ADD) {
                editor_new(&ed, num_levels);
                screen = SCREEN_EDIT;
            } else if (pick >= 0) {
                editor_edit(&ed, pick);
                screen = SCREEN_EDIT;
            }

        } else if (screen == SCREEN_EDIT) {
            if (editor_run(&ed, cmd) == EDITOR_EXIT) {
                num_levels = level_count();
                menu_level_select_reset(ed.index);
                screen = SCREEN_EDIT_SELECT;
            }

        } else {
            if (!loaded) {
                char path[64];
                level_path(current, path, sizeof(path));
                DrawText(TextFormat("Could not load %s", path), 40, 60, 24, RED);
            } else {
                ui_draw(&lv);
                DrawText(TextFormat("Level %d/%d", current + 1, num_levels),
                         300, 8, 24, DARKGRAY);

                if (arduino)
                    DrawText(TextFormat("Arduino: %s connected", port),
                             500, 12, 18, DARKGREEN);
                else
                    DrawText(TextFormat("Arduino: %s not found (keyboard mode)", port),
                             440, 12, 18, ORANGE);

                DrawText("Keypad: 2 up  8 down  4 left  6 right", 40, 480, 18, GRAY);
                DrawText("A undo  B redo  C restart  # confirm  7 prev  9 next  * levels",
                         40, 502, 18, GRAY);
                if (g_save_ok)
                    DrawText(TextFormat("Unlocked: %d of %d levels (auto-saved)",
                             g_save.unlocked + 1, num_levels), 40, 520, 18, GRAY);
                else
                    DrawText("SAVE FAILED - file path is in the window title",
                             40, 520, 18, RED);
                DrawText(TextFormat("Undo steps: %d   Redo steps: %d", undo.top, redo.top),
                         40, 540, 20, GRAY);
                DrawText(hint_msg, 40, 450, 20, RED);

                if (won) {
                    const char *msg = (current + 1 < num_levels)
                        ? "You win! Press # (or Enter) for the next level"
                        : "You win! All levels complete";
                    DrawText(msg, 40, 565, 28, DARKGREEN);
                }
            }

            if (menu_levels_button()) go_levels = true;
        }

        EndDrawing();

        if (go_levels) {
            menu_level_select_reset(g_save.last);
            screen = SCREEN_LEVELS;
        }
    }

    serial_close();
    stack_free(&undo);
    stack_free(&redo);
    CloseWindow();
    return 0;
}
