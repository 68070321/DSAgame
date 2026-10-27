#ifndef SOLVER_H
#define SOLVER_H

#include "level.h"
#include "input.h"
#include <stdbool.h>

// Why the solver stopped
typedef enum {
    SOLVE_FOUND,        // a solution was found (shortest in moves)
    SOLVE_UNWINNABLE,   // every reachable state was searched: no solution exists
    SOLVE_TOO_BIG,      // gave up at the search limit: unknown, might still be winnable
    SOLVE_ERROR         // out of memory
} SolveStatus;

typedef struct {
    Command *moves;
    int count;
    bool solved;            // same as status == SOLVE_FOUND
    SolveStatus status;
} Solution;

// Finds the shortest path to solve the level from its CURRENT state (so it also
// tells you when the player has made a move that can no longer be recovered from).
Solution solver_solve(const Level *initial_level);

// Frees the allocated moves in the solution
void solver_free_solution(Solution *sol);

#endif // SOLVER_H
