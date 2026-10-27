#include "input.h"

// Key mappings for the game and menus.
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

// Key mappings for the level editor.
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
