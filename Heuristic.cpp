// Heuristic.cpp

#include "stdafx.h"

#include "Heuristic.h"

#include "Board.h"
#include "Def.h"
#include "Utils.h"

void UpdateHeuristic(BoardItem* Board, int** CMH_Pointer, const int Move, const int Bonus)
{
    Board->HeuristicTable[Board->CurrentColor][PIECE(Board->Pieces[MOVE_FROM(Move)])][MOVE_TO(Move)] += Bonus - Board->HeuristicTable[Board->CurrentColor][PIECE(Board->Pieces[MOVE_FROM(Move)])][MOVE_TO(Move)] * ABS(Bonus) / MAX_HEURISTIC_SCORE;

#ifdef COUNTER_MOVE_HISTORY
    if (CMH_Pointer) {
        if (CMH_Pointer[0]) {
            CMH_Pointer[0][(PIECE(Board->Pieces[MOVE_FROM(Move)]) << 6) + MOVE_TO(Move)] += Bonus - CMH_Pointer[0][(PIECE(Board->Pieces[MOVE_FROM(Move)]) << 6) + MOVE_TO(Move)] * ABS(Bonus) / MAX_HEURISTIC_SCORE;
        }

        if (CMH_Pointer[1]) {
            CMH_Pointer[1][(PIECE(Board->Pieces[MOVE_FROM(Move)]) << 6) + MOVE_TO(Move)] += Bonus - CMH_Pointer[1][(PIECE(Board->Pieces[MOVE_FROM(Move)]) << 6) + MOVE_TO(Move)] * ABS(Bonus) / MAX_HEURISTIC_SCORE;
        }
    }
#endif // COUNTER_MOVE_HISTORY
}

void ClearHeuristic(BoardItem* Board)
{
    memset(Board->HeuristicTable, 0, sizeof(Board->HeuristicTable));

#ifdef COUNTER_MOVE_HISTORY
    memset(Board->CounterMoveHistoryTable, 0, sizeof(Board->CounterMoveHistoryTable));
#endif // COUNTER_MOVE_HISTORY
}

#ifdef COUNTER_MOVE_HISTORY
void SetCounterMoveHistoryPointer(BoardItem* Board, int** CMH_Pointer, const int Ply)
{
    HistoryItem* Info;

    if (Ply == 0) {
        CMH_Pointer[0] = CMH_Pointer[1] = NULL;

        return;
    }

    Info = &Board->MoveTable[Board->HalfMoveNumber - 1]; // Prev. move info

    if (Info->Type == MOVE_NULL) {
        CMH_Pointer[0] = CMH_Pointer[1] = NULL;

        return;
    }

    CMH_Pointer[0] = Board->CounterMoveHistoryTable[Info->PieceFrom][Info->To];

    if (Ply == 1) {
        CMH_Pointer[1] = NULL;

        return;
    }

    Info = &Board->MoveTable[Board->HalfMoveNumber - 2]; // Prev. prev. move info

    if (Info->Type == MOVE_NULL) {
        CMH_Pointer[1] = NULL;

        return;
    }

    CMH_Pointer[1] = Board->CounterMoveHistoryTable[Info->PieceFrom][Info->To];
}
#endif // COUNTER_MOVE_HISTORY

#ifdef KILLER_MOVE

void UpdateKillerMove(BoardItem* Board, const int Move, const int Ply)
{
    int TempMove;

    if (Board->KillerMoveTable[Ply][0] != Move) {
        TempMove = Board->KillerMoveTable[Ply][0];

        Board->KillerMoveTable[Ply][0] = Move;

        Board->KillerMoveTable[Ply][1] = TempMove;
    }
}

void ClearKillerMove(BoardItem* Board)
{
    memset(Board->KillerMoveTable, 0, sizeof(Board->KillerMoveTable));
}

#endif // KILLER_MOVE

#ifdef COUNTER_MOVE

void UpdateCounterMove(BoardItem* Board, const int Move, const int Ply)
{
    if (Ply == 0) {
        return;
    }

    HistoryItem* Info = &Board->MoveTable[Board->HalfMoveNumber - 1]; // Prev. move info

    if (Info->Type == MOVE_NULL) {
        return;
    }

    Board->CounterMoveTable[CHANGE_COLOR(Board->CurrentColor)][Info->PieceFrom][Info->To] = Move;
}

void ClearCounterMove(BoardItem* Board)
{
    memset(Board->CounterMoveTable, 0, sizeof(Board->CounterMoveTable));
}

#endif // COUNTER_MOVE