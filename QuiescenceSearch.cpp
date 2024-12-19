// QuiescenceSearch.cpp

#include "stdafx.h"

#include "QuiescenceSearch.h"

#include "Board.h"
#include "Def.h"
#include "Game.h"
#include "Gen.h"
#include "Hash.h"
#include "Move.h"
#include "NNUE2.h"
#include "SEE.h"
#include "Sort.h"
#include "Types.h"
#include "Utils.h"

int QuiescenceSearch(BoardItem* Board, int Alpha, int Beta, const int Depth, const int Ply, const BOOL IsPrincipal, const BOOL InCheck)
{
    assert(Alpha >= -INF);
    assert(Beta <= INF);
    assert(Alpha < Beta);
    assert(Ply >= 0 && Ply <= MAX_PLY);
    assert(InCheck == IsInCheck(Board, Board->CurrentColor));

    int GenMoveCount;
    MoveItem MoveList[MAX_GEN_MOVES];

#ifdef QUIESCENCE_USE_CHECK
    int LegalMoveCount = 0;
#endif // QUIESCENCE_USE_CHECK

    MoveItem BestMove = (MoveItem){ 0, 0, 0 };

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

#ifdef USE_STATISTIC
    ++Board->QuiescenceCount;
#endif // USE_STATISTIC

    if (omp_get_thread_num() == 0) { // Master thread
        if (
            CompletedDepth >= MIN_SEARCH_DEPTH
            && (Board->Nodes & 4095) == 0
            && Clock() >= TimeStop
        ) {
            StopSearch = TRUE;

            return 0;
        }
    }

    if (StopSearch) {
        return 0;
    }

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

#ifdef QUIESCENCE_MATE_DISTANCE_PRUNING
    Alpha = MAX(Alpha, -INF + Ply);
    Beta = MIN(Beta, INF - Ply - 1);

    if (Alpha >= Beta) {
        return Alpha;
    }
#endif // QUIESCENCE_MATE_DISTANCE_PRUNING

    if (Ply >= MAX_PLY) {
        return Evaluate(Board);
    }

    if (Board->HalfMoveNumber >= MAX_GAME_MOVES) {
        return Evaluate(Board);
    }

    LoadHash(Board->Hash, &HashDepth, Ply, &HashScore, &HashStaticScore, &HashMove, &HashFlag);

    if (InCheck || Depth >= 0) {
        QuiescenceHashDepth = 0;
    }
    else { // !InCheck && Depth < 0
        QuiescenceHashDepth = -1;
    }

    if (HashFlag) {
#ifdef USE_STATISTIC
        ++Board->HashCount;
#endif // USE_STATISTIC

        if (!IsPrincipal && HashDepth >= QuiescenceHashDepth) {
            if (
                (HashFlag == HASH_BETA && HashScore >= Beta)
                || (HashFlag == HASH_ALPHA && HashScore <= Alpha)
                || HashFlag == HASH_EXACT
            ) {
                return HashScore;
            }
        }
    }

#ifdef QUIESCENCE_USE_CHECK
    if (InCheck) {
        BestScore = StaticScore = -INF + Ply;

        GenMoveCount = 0;
        GenerateAllMoves(Board, NULL, MoveList, &GenMoveCount);
    }
    else {
#endif // QUIESCENCE_USE_CHECK
        if (HashFlag) {
            BestScore = StaticScore = HashStaticScore;

            if (
                (HashFlag == HASH_BETA && HashScore > BestScore)
                || (HashFlag == HASH_ALPHA && HashScore < BestScore)
                || HashFlag == HASH_EXACT
            ) {
                BestScore = HashScore;
            }
        }
        else {
            BestScore = StaticScore = Evaluate(Board);
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
#ifdef QUIESCENCE_USE_CHECK
    }
#endif // QUIESCENCE_USE_CHECK

    SetHashMoveSortValue(MoveList, GenMoveCount, HashMove);

    for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) {
        PrepareNextMove(MoveNumber, MoveList, GenMoveCount);

#ifdef QUIESCENCE_SEE_MOVE_PRUNING
        if (!InCheck && MoveList[MoveNumber].Move != HashMove) {
            if (SEE(Board, MoveList[MoveNumber].Type, MoveList[MoveNumber].Move) < 0) { // Bad capture/quiet move
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

#ifdef QUIESCENCE_USE_CHECK
        ++LegalMoveCount;
#endif // QUIESCENCE_USE_CHECK

        ++Board->Nodes;

        GiveCheck = IsInCheck(Board, Board->CurrentColor);

        Score = -QuiescenceSearch(Board, -Beta, -Alpha, Depth - 1, Ply + 1, IsPrincipal, GiveCheck);

        UnmakeMove(Board);

        if (StopSearch) {
            return 0;
        }

        if (Score > BestScore) {
            BestScore = Score;

            if (BestScore > Alpha) {
                BestMove = MoveList[MoveNumber];

                if (IsPrincipal && BestScore < Beta) {
                    Alpha = BestScore;
                }
                else { // !IsPrincipal || BestScore >= Beta
#ifdef USE_STATISTIC
                    ++Board->CutoffCount;
#endif // USE_STATISTIC

                    SaveHash(Board->Hash, QuiescenceHashDepth, Ply, BestScore, StaticScore, BestMove.Move, HASH_BETA);

                    return BestScore;
                }
            }
        }
    } // for

#ifdef QUIESCENCE_USE_CHECK
    if (InCheck && LegalMoveCount == 0) { // Checkmate
        return -INF + Ply;
    }
#endif // QUIESCENCE_USE_CHECK

    if (IsPrincipal && BestScore > OriginalAlpha) {
        HashFlag = HASH_EXACT;
    }
    else { // !IsPrincipal || BestScore <= OriginalAlpha
        HashFlag = HASH_ALPHA;
    }

    SaveHash(Board->Hash, QuiescenceHashDepth, Ply, BestScore, StaticScore, BestMove.Move, HashFlag);

    return BestScore;
}