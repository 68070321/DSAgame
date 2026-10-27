#ifndef EDITOR_H
#define EDITOR_H
#include <stdbool.h>
#include "input.h"
#include "level.h"

// Editor tools.
typedef enum {
    TOOL_WALL,      // Toggle a wall.
    TOOL_BOX,       // Toggle a box.
    TOOL_GOAL,      // Toggle a goal.
    TOOL_START,     // Set the player start.
    TOOL_ERASE      // Clear the cell.
} Tool;

typedef struct {
    Level lv;           // Level being edited.
    int index;          // Level file index.
    bool is_new;        // True until first save.
    int cx, cy;         // Cursor position.
    Tool brush;         // Selected tool.
    bool dirty;         // Has unsaved changes.
    bool back_armed;    // Warning was shown.
    bool status_error;  // Status message is an error.
    char status[96];
} Editor;

typedef enum {
    EDITOR_STAY,
    EDITOR_EXIT         // The player left the editor.
} EditorResult;

// Starts a new blank level.
void editor_new(Editor *ed, int index);

// Opens an existing level.
void editor_edit(Editor *ed, int index);

// Handles input and draws the editor once per frame.
EditorResult editor_run(Editor *ed, Command cmd);

#endif
