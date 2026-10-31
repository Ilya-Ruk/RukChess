// Heuristic.h

#pragma once

#ifndef HEURISTIC_H
#define HEURISTIC_H

#include "Board.h"
#include "Def.h"
#include "Utils.h"

#define MAX_HEURISTIC_SCORE     (1 << 14) // 16384

#define BONUS(Depth)            (SQR(MIN((Depth), 16)) * 64) // Max. 16384

void UpdateHeuristic(BoardItem* Board, SearchDataItem* SearchData, int** CMH_Pointer, const int Move, const int Bonus);
void ClearHeuristic(SearchDataItem* SearchData);

#ifdef COUNTER_MOVE_HISTORY
void SetCounterMoveHistoryPointer(BoardItem* Board, SearchDataItem* SearchData, int** CMH_Pointer, const int Ply);
#endif // COUNTER_MOVE_HISTORY

#ifdef KILLER_MOVE
void UpdateKillerMove(SearchDataItem* SearchData, const int Move, const int Ply);
void ClearKillerMove(SearchDataItem* SearchData);
#endif // KILLER_MOVE

#ifdef COUNTER_MOVE
void UpdateCounterMove(BoardItem* Board, SearchDataItem* SearchData, const int Move, const int Ply);
void ClearCounterMove(SearchDataItem* SearchData);
#endif // COUNTER_MOVE

#endif // !HEURISTIC_H