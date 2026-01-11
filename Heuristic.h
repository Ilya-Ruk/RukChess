// Heuristic.h

#pragma once

#ifndef HEURISTIC_H
#define HEURISTIC_H

#include "Board.h"
#include "Def.h"
#include "Utils.h"

#define MAX_HEURISTIC_SCORE     (1 << 13)

#define BONUS(Depth)            (SQR(MIN((Depth), 16)) * 32)

void UpdateHeuristic(BoardItem* Board, int** CMH_Pointer, const int Move, const int Bonus);
void ClearHeuristic(BoardItem* Board);

#ifdef COUNTER_MOVE_HISTORY
void SetCounterMoveHistoryPointer(BoardItem* Board, int** CMH_Pointer, const int Ply);
#endif // COUNTER_MOVE_HISTORY

#ifdef KILLER_MOVE
void UpdateKillerMove(BoardItem* Board, const int Move, const int Ply);
void ClearKillerMove(BoardItem* Board);
#endif // KILLER_MOVE

#ifdef COUNTER_MOVE
void UpdateCounterMove(BoardItem* Board, const int Move, const int Ply);
void ClearCounterMove(BoardItem* Board);
#endif // COUNTER_MOVE

#endif // !HEURISTIC_H