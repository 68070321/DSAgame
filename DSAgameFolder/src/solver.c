#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "solver.h"
#include "game.h"

// Limits keep the game responsive: the solver runs synchronously, so a huge
// search would freeze the window. When exceeded, the level counts as "unsolved".
#define MAX_NODES   400000
#define HASH_SIZE   (1 << 20)          // power of two, > 2 * MAX_NODES
#define EMPTY_SLOT  (-1)

// One BFS state. Nodes are stored in a growable array that doubles as the queue
// (nodes are appended in BFS order and consumed from `head`). Instead of every
// node carrying its own copy of the path, it stores just its parent and the move
// that led to it, and the path is rebuilt once at the end.
typedef struct {
    Level   state;
    int     parent;     // index of the parent node, -1 for the root
    Command cmd;        // move taken from the parent to get here
} Node;

static const struct { int dx; int dy; Command cmd; } DIRS[] = {
    {  0, -1, CMD_UP    },
    {  0,  1, CMD_DOWN  },
    { -1,  0, CMD_LEFT  },
    {  1,  0, CMD_RIGHT }
};

static bool is_box(char c) { return c == '$' || c == '*'; }

// A box on a non-goal tile with walls on two perpendicular sides can never move
// again. Out-of-bounds counts as wall.
static bool is_corner_deadlock(const Level *lv, int x, int y) {
    if (lv->grid[y][x] != '$') return false;     // '*' is on a goal: fine

    bool up    = (y <= 0)            || lv->grid[y - 1][x] == '#';
    bool down  = (y >= lv->height - 1) || lv->grid[y + 1][x] == '#';
    bool left  = (x <= 0)            || lv->grid[y][x - 1] == '#';
    bool right = (x >= lv->width - 1)  || lv->grid[y][x + 1] == '#';

    return (up && left) || (up && right) || (down && left) || (down && right);
}

// FNV-1a over the whole grid. The player is part of the grid ('@' / '+'), so
// grid contents alone identify a state. Unused cells are always ' ' (level_load
// memsets the grid), so hashing and comparing the full array is safe.
static unsigned int hash_grid(const Level *lv) {
    const unsigned char *p = (const unsigned char *)lv->grid;
    unsigned int h = 2166136261u;
    for (size_t i = 0; i < sizeof(lv->grid); i++) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

// Open-addressing set of node indices. Returns true if the state was already
// present; otherwise records it as node `new_index` and returns false.
static bool visit_if_new(int *table, const Node *nodes, const Level *lv, int new_index) {
    unsigned int slot = hash_grid(lv) & (HASH_SIZE - 1);
    while (table[slot] != EMPTY_SLOT) {
        if (memcmp(nodes[table[slot]].state.grid, lv->grid, sizeof(lv->grid)) == 0)
            return true;
        slot = (slot + 1) & (HASH_SIZE - 1);
    }
    table[slot] = new_index;
    return false;
}

Solution solver_solve(const Level *initial_level) {
    Solution sol = { .moves = NULL, .count = 0, .solved = false };

    Node *nodes = malloc(sizeof(Node) * (size_t)MAX_NODES);
    int  *table = malloc(sizeof(int) * (size_t)HASH_SIZE);
    if (!nodes || !table) {
        free(nodes);
        free(table);
        return sol;
    }
    for (int i = 0; i < HASH_SIZE; i++) table[i] = EMPTY_SLOT;

    int count = 0;      // nodes stored so far (tail of the queue)
    int head  = 0;      // next node to expand

    nodes[0].state  = *initial_level;
    nodes[0].parent = -1;
    nodes[0].cmd    = CMD_NONE;
    count = 1;
    visit_if_new(table, nodes, &nodes[0].state, 0);

    int found = -1;

    while (head < count && found < 0) {
        int cur = head++;

        if (game_is_won(&nodes[cur].state)) {
            found = cur;
            break;
        }

        for (int i = 0; i < 4; i++) {
            Level next = nodes[cur].state;

            // Is the player about to push a box? If so, remember where it ends up.
            int nx = next.player_x + DIRS[i].dx;
            int ny = next.player_y + DIRS[i].dy;
            bool pushing = nx >= 0 && ny >= 0 && nx < next.width && ny < next.height
                           && is_box(next.grid[ny][nx]);

            if (!game_move(&next, DIRS[i].dx, DIRS[i].dy)) continue;

            // Only a pushed box can have become newly stuck.
            if (pushing && is_corner_deadlock(&next, nx + DIRS[i].dx, ny + DIRS[i].dy))
                continue;

            if (count >= MAX_NODES) goto done;      // out of budget: give up

            nodes[count].state = next;
            if (visit_if_new(table, nodes, &nodes[count].state, count)) continue;

            nodes[count].parent = cur;
            nodes[count].cmd    = DIRS[i].cmd;
            count++;
        }
    }

done:
    if (found >= 0) {
        // Walk back to the root to count, then fill the move list in order.
        int len = 0;
        for (int n = found; nodes[n].parent != -1; n = nodes[n].parent) len++;

        sol.solved = true;
        sol.count  = len;
        if (len > 0) {
            sol.moves = malloc(sizeof(Command) * (size_t)len);
            if (!sol.moves) {                       // out of memory: report failure
                sol.solved = false;
                sol.count  = 0;
            } else {
                int k = len;
                for (int n = found; nodes[n].parent != -1; n = nodes[n].parent)
                    sol.moves[--k] = nodes[n].cmd;
            }
        }
    }

    free(nodes);
    free(table);
    return sol;
}

void solver_free_solution(Solution *sol) {
    if (!sol) return;
    free(sol->moves);
    sol->moves  = NULL;
    sol->count  = 0;
    sol->solved = false;
}
