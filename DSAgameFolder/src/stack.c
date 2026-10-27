#include "stack.h"
#include <stdlib.h>

void stack_init(Stack *s) {
    s->items = NULL;
    s->top = 0;
    s->capacity = 0;
}

void stack_free(Stack *s) {
    free(s->items);
    stack_init(s);
}

void stack_clear(Stack *s) {
    s->top = 0;
}

bool stack_push(Stack *s, const Level *lv) {
    if (s->top == s->capacity) {                       // Grow the stack when full.
        int new_cap = s->capacity ? s->capacity * 2 : 16;
        Level *p = realloc(s->items, (size_t)new_cap * sizeof(Level));
        if (!p) return false;
        s->items = p;
        s->capacity = new_cap;
    }
    s->items[s->top++] = *lv;
    return true;
}

bool stack_pop(Stack *s, Level *out) {
    if (s->top == 0) return false;
    *out = s->items[--s->top];
    return true;
}
