#ifndef SOLVER_H
#define SOLVER_H

#include "level.h"
#include "input.h"
#include <stdbool.h>

// Solver result.
typedef enum {
    SOLVE_FOUND,        // A solution was found.
    SOLVE_UNWINNABLE,   // No solution exists.
    SOLVE_TOO_BIG,      // Search limit reached; result unknown.
    SOLVE_ERROR         // Not enough memory.
} SolveStatus;

typedef struct {
    Command *moves;
    int count;
    bool solved;            // True when a solution was found.
    SolveStatus status;
} Solution;

// Finds the shortest solution from the current level state.
Solution solver_solve(const Level *initial_level);

// Frees the solution moves.
void solver_free_solution(Solution *sol);

#endif
