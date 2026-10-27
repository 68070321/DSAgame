#ifndef SOLVER_H
#define SOLVER_H

#include "level.h"
#include "input.h"
#include <stdbool.h>

typedef struct {
    Command *moves;
    int count;
    bool solved;
} Solution;

// Finds the shortest path to solve the level
Solution solver_solve(const Level *initial_level);

// Frees the allocated moves in the solution
void solver_free_solution(Solution *sol);

#endif // SOLVER_H