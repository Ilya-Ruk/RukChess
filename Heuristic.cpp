// Heuristic.cpp

#include "stdafx.h"

#include "Heuristic.h"

#include "Board.h"
#include "Def.h"
#include "Utils.h"

void UpdateHeuristic(const BoardItem* Board, SearchDataItem* SearchData, int** CMH_Pointer, const int Move, const int Bonus)
{
    SearchData->HeuristicTable[Board->CurrentColor][PIECE_TYPE(Board->Pieces[MOVE_FROM(Move)])][MOVE_TO(Move)] += Bonus - SearchData->HeuristicTable[Board->CurrentColor][PIECE_TYPE(Board->Pieces[MOVE_FROM(Move)])][MOVE_TO(Move)] * ABS(Bonus) / MAX_HEURISTIC_SCORE;

#ifdef COUNTER_MOVE_HISTORY
    if (CMH_Pointer != NULL) {
        if (CMH_Pointer[0] != NULL) {
            CMH_Pointer[0][(PIECE_TYPE(Board->Pieces[MOVE_FROM(Move)]) << 6) + MOVE_TO(Move)] += Bonus - CMH_Pointer[0][(PIECE_TYPE(Board->Pieces[MOVE_FROM(Move)]) << 6) + MOVE_TO(Move)] * ABS(Bonus) / MAX_HEURISTIC_SCORE;
        }

        if (CMH_Pointer[1] != NULL) {
            CMH_Pointer[1][(PIECE_TYPE(Board->Pieces[MOVE_FROM(Move)]) << 6) + MOVE_TO(Move)] += Bonus - CMH_Pointer[1][(PIECE_TYPE(Board->Pieces[MOVE_FROM(Move)]) << 6) + MOVE_TO(Move)] * ABS(Bonus) / MAX_HEURISTIC_SCORE;
        }
    }
#endif // COUNTER_MOVE_HISTORY
}

void ClearHeuristic(SearchDataItem* SearchData)
{
    memset(SearchData->HeuristicTable, 0, sizeof(SearchData->HeuristicTable));

#ifdef COUNTER_MOVE_HISTORY
    memset(SearchData->CounterMoveHistoryTable, 0, sizeof(SearchData->CounterMoveHistoryTable));
#endif // COUNTER_MOVE_HISTORY
}

#ifdef COUNTER_MOVE_HISTORY
void SetCounterMoveHistoryPointer(const BoardItem* Board, const SearchDataItem* SearchData, const int** CMH_Pointer, const int Ply)
{
    if (Ply == 0) {
        CMH_Pointer[0] = NULL;
        CMH_Pointer[1] = NULL;

        return;
    }

    const HistoryItem* Info_1 = &Board->MoveTable[Board->HalfMoveNumber - 1]; // Prev. move info

    if (Info_1->Type == MOVE_NULL) {
        CMH_Pointer[0] = NULL;
        CMH_Pointer[1] = NULL;

        return;
    }

    CMH_Pointer[0] = SearchData->CounterMoveHistoryTable[Info_1->PieceTypeFrom][Info_1->To];

    if (Ply == 1) {
        CMH_Pointer[1] = NULL;

        return;
    }

    const HistoryItem* Info_2 = &Board->MoveTable[Board->HalfMoveNumber - 2]; // Prev. prev. move info

    if (Info_2->Type == MOVE_NULL) {
        CMH_Pointer[1] = NULL;

        return;
    }

    CMH_Pointer[1] = SearchData->CounterMoveHistoryTable[Info_2->PieceTypeFrom][Info_2->To];
}
#endif // COUNTER_MOVE_HISTORY

#ifdef KILLER_MOVE

void UpdateKillerMove(SearchDataItem* SearchData, const int Move, const int Ply)
{
    if (SearchData->KillerMoveTable[Ply][0] != Move) {
        const int TempMove = SearchData->KillerMoveTable[Ply][0];

        SearchData->KillerMoveTable[Ply][0] = Move;
        SearchData->KillerMoveTable[Ply][1] = TempMove;
    }
}

void ClearKillerMove(SearchDataItem* SearchData)
{
    memset(SearchData->KillerMoveTable, 0, sizeof(SearchData->KillerMoveTable));
}

#endif // KILLER_MOVE

#ifdef COUNTER_MOVE

void UpdateCounterMove(const BoardItem* Board, SearchDataItem* SearchData, const int Move, const int Ply)
{
    if (Ply == 0) {
        return;
    }

    const HistoryItem* Info_1 = &Board->MoveTable[Board->HalfMoveNumber - 1]; // Prev. move info

    if (Info_1->Type == MOVE_NULL) {
        return;
    }

    SearchData->CounterMoveTable[CHANGE_COLOR(Board->CurrentColor)][Info_1->PieceTypeFrom][Info_1->To] = Move;
}

void ClearCounterMove(SearchDataItem* SearchData)
{
    memset(SearchData->CounterMoveTable, 0, sizeof(SearchData->CounterMoveTable));
}

#endif // COUNTER_MOVE