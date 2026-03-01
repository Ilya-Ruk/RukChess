// MCTS.h

#pragma once

#ifndef MCTS_H
#define MCTS_H

#include "Board.h"
#include "Def.h"
#include "Types.h"

typedef struct NodeMCTS {
    struct NodeMCTS* Parent;
    struct NodeMCTS* Children[MAX_GEN_MOVES];

    int ChildCount;

    MoveItem Move;

    BOOL IsTerminal;
    BOOL IsFullyExpanded; // For terminal node always TRUE

    int GameResult; // Just for terminal node

    int NextMoveNumber;

    int N;
    double Q;
} NodeItemMCTS; // 2104 bytes

void MonteCarloTreeSearch(BoardItem* Board, MoveItem* BestMoves, int* BestScore);

#endif // !MCTS_H