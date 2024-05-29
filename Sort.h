// Sort.h

#pragma once

#ifndef SORT_H
#define SORT_H

#include "Board.h"
#include "Def.h"

#define SORT_HASH_MOVE_VALUE            (1 << 30)

#define SORT_PAWN_PROMOTE_MOVE_BONUS    (1 << 29)
#define SORT_CAPTURE_MOVE_BONUS         (1 << 28)

#define SORT_KILLER_MOVE_1_VALUE        (1 << 27)
#define SORT_KILLER_MOVE_2_VALUE        (SORT_KILLER_MOVE_1_VALUE - 1)

#define SORT_COUNTER_MOVE_VALUE         (SORT_KILLER_MOVE_2_VALUE - 1)

void SetHashMoveSortValue(MoveItem* GenMoveList, const int GenMoveCount, const int HashMove);

#ifdef KILLER_MOVE
void SetKillerMove1SortValue(const BoardItem* Board, const int Ply, MoveItem* GenMoveList, const int GenMoveCount, const int HashMove);
void SetKillerMove2SortValue(const BoardItem* Board, const int Ply, MoveItem* GenMoveList, const int GenMoveCount, const int HashMove);
#endif // KILLER_MOVE

#ifdef COUNTER_MOVE
void SetCounterMoveSortValue(const BoardItem* Board, const int Ply, MoveItem* GenMoveList, const int GenMoveCount, const int HashMove);
#endif // COUNTER_MOVE

void PrepareNextMove(const int StartIndex, MoveItem* GenMoveList, const int GenMoveCount);

#endif // !SORT_H