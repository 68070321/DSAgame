#ifndef STACK_H
#define STACK_H
#include <stdbool.h>
#include "level.h"

// Stack of level snapshots used for undo and redo.
typedef struct {
    Level *items;
    int top;        // Number of saved levels.
    int capacity;   // Stack size.
} Stack;

void stack_init(Stack *s);
void stack_free(Stack *s);
void stack_clear(Stack *s);                       // Removes all saved levels.
bool stack_push(Stack *s, const Level *lv);       // Saves a level.
bool stack_pop(Stack *s, Level *out);             // Gets the last saved level.

#endif
