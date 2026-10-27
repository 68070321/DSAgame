#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "solver.h"
#include "game.h"

// Search limits keep the game responsive.
#define MAX_NODES   400000
#define HASH_SIZE   (1 << 20)
#define EMPTY_SLOT  (-1)

// Each search state stores the move and its parent.
typedef struct {
    Level   state;
    int     parent;     // Previous state, or -1 for the start.
    Command cmd;        // Move to reach this state.
} Node;

static const struct { int dx; int dy; Command cmd; } DIRS[] = {
    {  0, -1, CMD_UP    },
    {  0,  1, CMD_DOWN  },
    { -1,  0, CMD_LEFT  },
    {  1,  0, CMD_RIGHT }
};

static bool is_box(char c) { return c == '$' || c == '*'; }

// Checks whether a box is stuck in a corner.
static bool is_corner_deadlock(const Level *lv, int x, int y) {
    if (lv->grid[y][x] != '$') return false;

    bool up    = (y <= 0)            || lv->grid[y - 1][x] == '#';
    bool down  = (y >= lv->height - 1) || lv->grid[y + 1][x] == '#';
    bool left  = (x <= 0)            || lv->grid[y][x - 1] == '#';
    bool right = (x >= lv->width - 1)  || lv->grid[y][x + 1] == '#';

    return (up && left) || (up && right) || (down && left) || (down && right);
}

// Hashes the grid to identify a search state.
static unsigned int hash_grid(const Level *lv) {
    const unsigned char *p = (const unsigned char *)lv->grid;
    unsigned int h = 2166136261u;
    for (size_t i = 0; i < sizeof(lv->grid); i++) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

// Tracks states that have already been searched.
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
    Solution sol = {
        .moves = NULL,
        .count = 0,
        .solved = false,
        .status = SOLVE_ERROR
    };

    Node *nodes = malloc(sizeof(Node) * (size_t)MAX_NODES);
    int  *table = malloc(sizeof(int) * (size_t)HASH_SIZE);
    if (!nodes || !table) {
        free(nodes);
        free(table);
        return sol;
    }
    sol.status = SOLVE_UNWINNABLE;
    for (int i = 0; i < HASH_SIZE; i++) table[i] = EMPTY_SLOT;

    int count = 0;
    int head  = 0;

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

            // Remember the box's new position after a push.
            int nx = next.player_x + DIRS[i].dx;
            int ny = next.player_y + DIRS[i].dy;
            bool pushing = nx >= 0 && ny >= 0 && nx < next.width && ny < next.height
                           && is_box(next.grid[ny][nx]);

            if (!game_move(&next, DIRS[i].dx, DIRS[i].dy)) continue;

            // Only check a box after it moves.
            if (pushing && is_corner_deadlock(&next, nx + DIRS[i].dx, ny + DIRS[i].dy))
                continue;

            if (count >= MAX_NODES) {
                sol.status = SOLVE_TOO_BIG;
                goto done;
            }

            nodes[count].state = next;
            if (visit_if_new(table, nodes, &nodes[count].state, count)) continue;

            nodes[count].parent = cur;
            nodes[count].cmd    = DIRS[i].cmd;
            count++;
        }
    }

done:
    if (found >= 0) {
        // Rebuild the moves by following parent states.
        int len = 0;
        for (int n = found; nodes[n].parent != -1; n = nodes[n].parent) len++;

        sol.count  = len;
        if (len > 0) {
            sol.moves = malloc(sizeof(Command) * (size_t)len);
            if (!sol.moves) {
                sol.count  = 0;
                sol.status = SOLVE_ERROR;
            } else {
                int k = len;
                for (int n = found; nodes[n].parent != -1; n = nodes[n].parent)
                    sol.moves[--k] = nodes[n].cmd;
            }
        }
        if (len == 0 || sol.moves) {
            sol.solved = true;
            sol.status = SOLVE_FOUND;
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
