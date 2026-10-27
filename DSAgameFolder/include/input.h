#ifndef INPUT_H
#define INPUT_H

// Commands from the keyboard or keypad.
typedef enum {
    CMD_NONE,
    CMD_UP,
    CMD_DOWN,
    CMD_LEFT,
    CMD_RIGHT,
    CMD_UNDO,
    CMD_REDO,
    CMD_RESTART,
    CMD_CONFIRM,    // Confirm a choice.
    CMD_PREV,       // Previous level.
    CMD_NEXT,       // Next unlocked level.
    CMD_LEVELS,     // Open level select.
    CMD_BACK,       // Go back.
    CMD_SOLVE,      // Solve the puzzle.

    // Commands for the level editor.
    CMD_WALL,
    CMD_BOX,
    CMD_GOAL,
    CMD_START,
    CMD_ERASE,
    CMD_NARROWER,
    CMD_WIDER,
    CMD_SHORTER,
    CMD_TALLER,
    CMD_SAVE,
    CMD_CHECK
} Command;

// Converts a key to a game command.
Command input_from_key(int c);

// Converts a key to an editor command.
Command input_from_key_editor(int c);

#endif
