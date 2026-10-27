#ifndef INPUT_H
#define INPUT_H

// Everything the player can ask the game to do, whatever device it came from
typedef enum {
    CMD_NONE,
    CMD_UP,
    CMD_DOWN,
    CMD_LEFT,
    CMD_RIGHT,
    CMD_UNDO,
    CMD_REDO,
    CMD_RESTART,
    CMD_CONFIRM,    // next level after a win, or "select" in the menus
    CMD_PREV,       // previous level
    CMD_NEXT,       // next unlocked level
    CMD_LEVELS,     // open the level select screen
    CMD_BACK,       // go back (level select -> menu, game -> level select)
    CMD_SOLVE,        //solves the puzzle

    // level editor only
    CMD_WALL,       // toggle a wall on the cursor cell
    CMD_BOX,        // toggle a box
    CMD_GOAL,       // toggle a goal
    CMD_START,      // move the player start here
    CMD_ERASE,      // clear the cell
    CMD_NARROWER,   // map one column narrower
    CMD_WIDER,      // map one column wider
    CMD_SHORTER,    // map one row shorter
    CMD_TALLER,     // map one row taller
    CMD_SAVE,        // save the level
    CMD_CHECK       // check if the level is winnable
} Command;

// Maps one character sent by the Arduino keypad to a command (game and menus).
// Unknown characters (including '\r' and '\n') give CMD_NONE.
Command input_from_key(int c);

// Same, but with the keypad layout used inside the level editor.
Command input_from_key_editor(int c);

#endif
