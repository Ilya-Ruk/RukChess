// QuiescenceSearch.cpp

#include "stdafx.h"

#include "QuiescenceSearch.h"

#include "Board.h"
#include "Def.h"
#include "Evaluate.h"
#include "Game.h"
#include "Gen.h"
#include "Hash.h"
#include "Move.h"
#include "SEE.h"
#include "Sort.h"
#include "Types.h"
#include "Utils.h"

int QuiescenceSearch(BoardItem* Board, int Alpha, int Beta, const int Depth, const int Ply, MoveItem* BestMoves, const BOOL IsPrincipal, const BOOL InCheck)
{
    int GenMoveCount;
    MoveItem MoveList[MAX_GEN_MOVES];

    int LegalMoveCount = 0;

    MoveItem BestMove = (MoveItem){ 0, 0, 0 };

    MoveItem TempBestMoves[MAX_PLY];

    int HashScore = -INF;
    int HashStaticScore = -INF;
    int HashMove = 0;
    int HashDepth = -MAX_PLY;
    int HashFlag = 0;

    int QuiescenceHashDepth;

    int OriginalAlpha = Alpha;

    int Score;
    int BestScore;
    int StaticScore;

    BOOL GiveCheck;

#ifdef DEBUG_STATISTIC
    ++Board->QuiescenceCount;
#endif // DEBUG_STATISTIC

    if (
        omp_get_thread_num() == 0 // Master thread
        && (Board->Nodes & 65535) == 0
        && CompletedDepth >= MIN_SEARCH_DEPTH
        && Clock() >= TimeStop
    ) {
        StopSearch = TRUE;

        return 0;
    }

    if (StopSearch) {
        return 0;
    }

#ifdef QUIESCENCE_MATE_DISTANCE_PRUNING
    Alpha = MAX(Alpha, -INF + Ply);
    Beta = MIN(Beta, INF - Ply - 1);

    if (Alpha >= Beta) {
        return Alpha;
    }
#endif // QUIESCENCE_MATE_DISTANCE_PRUNING

    if (IsInsufficientMaterial(Board)) {
        return 0;
    }

    if (Board->FiftyMove >= 100) {
        if (InCheck) {
            if (!HasLegalMoves(Board)) { // Checkmate
                return -INF + Ply;
            }
        }

        return 0;
    }

    if (PositionRepeat1(Board)) {
        return 0;
    }

    if (Ply >= MAX_PLY) {
        return (int)Evaluate(Board);
    }

    if (Board->HalfMoveNumber >= MAX_GAME_MOVES) {
        return (int)Evaluate(Board);
    }

    LoadHash(Board->Hash, &HashDepth, Ply, &HashScore, &HashStaticScore, &HashMove, &HashFlag);

    if (InCheck || Depth >= 0) {
        QuiescenceHashDepth = 0;
    }
    else { // !InCheck && Depth < 0
        QuiescenceHashDepth = -1;
    }

    if (HashFlag) {
#ifdef DEBUG_STATISTIC
        ++Board->HashCount;
#endif // DEBUG_STATISTIC

        if (!IsPrincipal && HashDepth >= QuiescenceHashDepth) {
            if (
                (HashFlag == HASH_BETA && HashScore >= Beta)
                || (HashFlag == HASH_ALPHA && HashScore <= Alpha)
                || (HashFlag == HASH_EXACT)
            ) {
                return HashScore;
            }
        }
    }

#ifdef QUIESCENCE_CHECK_EXTENSION
    if (InCheck) {
        BestScore = StaticScore = -INF + Ply;

        GenMoveCount = 0;
        GenerateAllMoves(Board, NULL, MoveList, &GenMoveCount);
    }
    else {
#endif // QUIESCENCE_CHECK_EXTENSION
        if (HashFlag) {
            BestScore = StaticScore = HashStaticScore;

            if (
                (HashFlag == HASH_BETA && HashScore > BestScore)
                || (HashFlag == HASH_ALPHA && HashScore < BestScore)
                || (HashFlag == HASH_EXACT)
            ) {
                BestScore = HashScore;
            }
        }
        else {
            BestScore = StaticScore = (int)Evaluate(Board);
        }

        if (BestScore >= Beta) {
            if (!HashFlag) {
                SaveHash(Board->Hash, -MAX_PLY, 0, 0, StaticScore, 0, HASH_STATIC_SCORE);
            }

            return BestScore;
        }

        if (IsPrincipal && BestScore > Alpha) {
            Alpha = BestScore;
        }

        GenMoveCount = 0;
        GenerateCaptureMoves(Board, NULL, MoveList, &GenMoveCount);
#ifdef QUIESCENCE_CHECK_EXTENSION
    }
#endif // QUIESCENCE_CHECK_EXTENSION

    SetHashMoveSortValue(MoveList, GenMoveCount, HashMove);

    for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) {
        PrepareNextMove(MoveNumber, MoveList, GenMoveCount);

#ifdef QUIESCENCE_SEE_MOVE_PRUNING
        if (
            !InCheck
            && MoveList[MoveNumber].Move != HashMove
        ) {
            if (CaptureSEE(Board, MOVE_FROM(MoveList[MoveNumber].Move), MOVE_TO(MoveList[MoveNumber].Move), MOVE_PROMOTE_PIECE(MoveList[MoveNumber].Move), MoveList[MoveNumber].Type) < 0) { // Bad capture/quiet move
                continue; // Next move
            }
        }
#endif // QUIESCENCE_SEE_MOVE_PRUNING

        MakeMove(Board, MoveList[MoveNumber]);

#ifdef HASH_PREFETCH
        Prefetch(Board->Hash);
#endif // HASH_PREFETCH

        if (IsInCheck(Board, CHANGE_COLOR(Board->CurrentColor))) { // Illegal move
            UnmakeMove(Board);

            continue; // Next move
        }

        ++LegalMoveCount;

        ++Board->Nodes;

        GiveCheck = IsInCheck(Board, Board->CurrentColor);

        TempBestMoves[0] = (MoveItem){ 0, 0, 0 };

        Score = -QuiescenceSearch(Board, -Beta, -Alpha, Depth - 1, Ply + 1, TempBestMoves, IsPrincipal, GiveCheck);

        UnmakeMove(Board);

        if (StopSearch) {
            return 0;
        }

        if (Score > BestScore) {
            BestScore = Score;

            if (BestScore > Alpha) {
                BestMove = MoveList[MoveNumber];

                if (IsPrincipal) {
                    SaveBestMoves(BestMoves, BestMove, TempBestMoves);
                }

                if (IsPrincipal && BestScore < Beta) {
                    Alpha = BestScore;
                }
                else { // !IsPrincipal || BestScore >= Beta
#ifdef DEBUG_STATISTIC
                    ++Board->CutoffCount;
#endif // DEBUG_STATISTIC

                    SaveHash(Board->Hash, QuiescenceHashDepth, Ply, BestScore, StaticScore, BestMove.Move, HASH_BETA);

                    return BestScore;
                }
            }
        }
    } // for

#ifdef QUIESCENCE_CHECK_EXTENSION
    if (InCheck && LegalMoveCount == 0) { // Checkmate
        return -INF + Ply;
    }
#endif // QUIESCENCE_CHECK_EXTENSION

    if (IsPrincipal && BestScore > OriginalAlpha) {
        HashFlag = HASH_EXACT;
    }
    else { // !IsPrincipal || BestScore <= OriginalAlpha
        HashFlag = HASH_ALPHA;
    }

    SaveHash(Board->Hash, QuiescenceHashDepth, Ply, BestScore, StaticScore, BestMove.Move, HashFlag);

    return BestScore;
}