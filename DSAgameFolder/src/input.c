#include "input.h"

// Keypad layout in the game and menus (4x4 membrane keypad):
//
//      1   [2 up]     3    [A undo]
//   [4 left] 5   [6 right] [B redo]
//   [7 prev] [8 down] [9 next] [C restart]
//   [* levels] [0 back] [# confirm]   D
//
// Changing the layout only means editing these tables. The Arduino just sends
// the raw key, so it never needs to be reflashed.
Command input_from_key(int c) {
    switch (c) {
        case '2': return CMD_UP;
        case '8': return CMD_DOWN;
        case '4': return CMD_LEFT;
        case '6': return CMD_RIGHT;
        case 'A': return CMD_UNDO;
        case 'B': return CMD_REDO;
        case 'C': return CMD_RESTART;
        case '#': return CMD_CONFIRM;
        case '7': return CMD_PREV;
        case '9': return CMD_NEXT;
        case '*': return CMD_LEVELS;
        case '0': return CMD_BACK;
        case 'D': return CMD_SOLVE;
        default:  return CMD_NONE;
    }
}

// Keypad layout inside the level editor. The movement keys stay the same, and
// the tools sit on the odd digits so they surround the arrow cross:
//
//   [1 wall]   [2 up]     [3 box]     [A narrower]
//   [4 left]   [5 goal]   [6 right]   [B wider]
//   [7 start]  [8 down]   [9 erase]   [C shorter]
//      *       [0 back]   [# save]    [D taller]
Command input_from_key_editor(int c) {
    switch (c) {
        case '2': return CMD_UP;
        case '8': return CMD_DOWN;
        case '4': return CMD_LEFT;
        case '6': return CMD_RIGHT;
        case '1': return CMD_WALL;
        case '3': return CMD_BOX;
        case '5': return CMD_GOAL;
        case '7': return CMD_START;
        case '9': return CMD_ERASE;
        case 'A': return CMD_NARROWER;
        case 'B': return CMD_WIDER;
        case 'C': return CMD_SHORTER;
        case 'D': return CMD_TALLER;
        case '#': return CMD_SAVE;
        case '*': return CMD_CHECK;
        case '0': return CMD_BACK;
        default:  return CMD_NONE;
    }
}
