// Search.cpp

#include "stdafx.h"

#include "Search.h"

#include "Board.h"
#include "Def.h"
#include "Game.h"
#include "Gen.h"
#include "Hash.h"
#include "Heuristic.h"
#include "Move.h"
#include "NNUE2.h"
#include "QuiescenceSearch.h"
#include "SEE.h"
#include "Sort.h"
#include "Types.h"
#include "Utils.h"

#ifdef FUTILITY_PRUNING
int FutilityMargin(const int Depth) // Hakkapeliitta
{
    return (25 * Depth + 100);
}
#endif // FUTILITY_PRUNING

#ifdef REVERSE_FUTILITY_PRUNING
int ReverseFutilityMargin(const int Depth) // Hakkapeliitta
{
    return (50 * Depth + 100);
}
#endif // REVERSE_FUTILITY_PRUNING

#ifdef RAZORING
int RazoringMargin(const int Depth) // Hakkapeliitta
{
    return (50 * Depth + 50);
}
#endif // RAZORING

int Search(BoardItem* Board, int Alpha, int Beta, int Depth, const int Ply, MoveItem* BestMoves, const BOOL IsPrincipal, const BOOL InCheck, const BOOL UsePruning, const int SkipMove)
{
    assert(Alpha >= -INF);
    assert(Beta <= INF);
    assert(Alpha < Beta);
    assert(Ply >= 0 && Ply <= MAX_PLY);
    assert(InCheck == IsInCheck(Board, Board->CurrentColor));

    int GenMoveCount;
    MoveItem MoveList[MAX_GEN_MOVES];

    int QuietMoveCount = 0;
    int QuietMoveList[MAX_GEN_MOVES]; // Move only

    int LegalMoveCount = 0;

    MoveItem BestMove = (MoveItem){ 0, 0, 0 };

    MoveItem TempBestMoves[MAX_PLY];

    int HashScore = -INF;
    int HashStaticScore = -INF;
    int HashMove = 0;
    int HashDepth = -MAX_PLY;
    int HashFlag = 0;

    int Score;
    int BestScore;
    int StaticScore;

    BOOL GiveCheck;

    int Extension;

    int NewDepth;

#if defined(REVERSE_FUTILITY_PRUNING) || defined(NULL_MOVE_PRUNING)
    BOOL NonPawnMaterial = !!(Board->BB_Pieces[Board->CurrentColor][KNIGHT] | Board->BB_Pieces[Board->CurrentColor][BISHOP] | Board->BB_Pieces[Board->CurrentColor][ROOK] | Board->BB_Pieces[Board->CurrentColor][QUEEN]);
#endif // REVERSE_FUTILITY_PRUNING || NULL_MOVE_PRUNING

#ifdef RAZORING
    int RazoringAlpha;
#endif // RAZORING

#ifdef NULL_MOVE_PRUNING
    int NullMoveReduction;
#endif // NULL_MOVE_PRUNING

#ifdef PROBCUT
    int BetaCut;
#endif // PROBCUT

#ifdef BAD_CAPTURE_LAST
    int SEE_Value;
#endif // BAD_CAPTURE_LAST

#ifdef LATE_MOVE_REDUCTION
    int LateMoveReduction;
#endif // LATE_MOVE_REDUCTION

#ifdef SINGULAR_EXTENSION
    int SingularBeta;
#endif // SINGULAR_EXTENSION

    int* CMH_Pointer[2];

    if (Depth <= 0) {
        return QuiescenceSearch(Board, Alpha, Beta, 0, Ply, IsPrincipal, InCheck);
    }

    if (omp_get_thread_num() == 0) { // Master thread
        if (
            Ply > 0
            && CompletedDepth >= MIN_SEARCH_DEPTH
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

    if (Ply > 0) {
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

#ifdef MATE_DISTANCE_PRUNING
        Alpha = MAX(Alpha, -INF + Ply);
        Beta = MIN(Beta, INF - Ply - 1);

        if (Alpha >= Beta) {
            return Alpha;
        }
#endif // MATE_DISTANCE_PRUNING
    }

    if (Ply >= MAX_PLY) {
        return Evaluate(Board);
    }

    if (Board->HalfMoveNumber >= MAX_GAME_MOVES) {
        return Evaluate(Board);
    }

    if (IsPrincipal && Board->SelDepth < Ply + 1) {
        Board->SelDepth = Ply + 1;
    }

#ifdef COUNTER_MOVE_HISTORY
    SetCounterMoveHistoryPointer(Board, CMH_Pointer, Ply);
#else
    CMH_Pointer[0] = NULL;
    CMH_Pointer[1] = NULL;
#endif // COUNTER_MOVE_HISTORY

    if (!SkipMove) {
        LoadHash(Board->Hash, &HashDepth, Ply, &HashScore, &HashStaticScore, &HashMove, &HashFlag);

        if (HashFlag) {
#ifdef USE_STATISTIC
            ++Board->HashCount;
#endif // USE_STATISTIC

            if (!IsPrincipal && HashDepth >= Depth) {
                if (
                    (HashFlag == HASH_BETA && HashScore >= Beta)
                    || (HashFlag == HASH_ALPHA && HashScore <= Alpha)
                    || HashFlag == HASH_EXACT
                ) {
                    if (HashMove) {
                        if (
                            Board->Pieces[MOVE_TO(HashMove)] == NO_PIECE
                            && (
                                PIECE_TYPE(Board->Pieces[MOVE_FROM(HashMove)]) != PAWN
                                || (
                                    MOVE_TO(HashMove) != Board->PassantSquare
                                    && RANK(MOVE_TO(HashMove)) != 0
                                    && RANK(MOVE_TO(HashMove)) != 7
                                )
                            )
                        ) { // Not capture/promote move
                            if (HashFlag == HASH_BETA) {
                                UpdateHeuristic(Board, CMH_Pointer, HashMove, BONUS(Depth));

#ifdef KILLER_MOVE
                                UpdateKillerMove(Board, HashMove, Ply);
#endif // KILLER_MOVE

#ifdef COUNTER_MOVE
                                UpdateCounterMove(Board, HashMove, Ply);
#endif // COUNTER_MOVE
                            }
                            else if (HashFlag == HASH_ALPHA) {
                                UpdateHeuristic(Board, CMH_Pointer, HashMove, -BONUS(Depth));
                            }
                        }
                    }

                    return HashScore;
                } // if
            } // if
        } // if
    } // if

    if (InCheck) {
        StaticScore = -INF + Ply;
    }
    else {
        if (HashFlag) {
            StaticScore = HashStaticScore;
        }
        else {
            StaticScore = Evaluate(Board);

            if (!SkipMove) {
                SaveHash(Board->Hash, -MAX_PLY, 0, 0, StaticScore, 0, HASH_STATIC_SCORE);
            }
        }
    }

#if defined(REVERSE_FUTILITY_PRUNING) || defined(RAZORING) || defined(NULL_MOVE_PRUNING) || defined(PROBCUT)
    if (UsePruning && !IsPrincipal && !InCheck) {
#ifdef REVERSE_FUTILITY_PRUNING
        if (NonPawnMaterial && Depth <= 5 && (StaticScore - ReverseFutilityMargin(Depth)) >= Beta) { // Hakkapeliitta
            return (StaticScore - ReverseFutilityMargin(Depth));
        }
#endif // REVERSE_FUTILITY_PRUNING

#ifdef RAZORING
        if (Depth <= 3 && (StaticScore + RazoringMargin(Depth)) <= Alpha) { // Hakkapeliitta
            RazoringAlpha = Alpha - RazoringMargin(Depth);

            // Zero window quiescence search
            Score = QuiescenceSearch(Board, RazoringAlpha, RazoringAlpha + 1, 0, Ply, FALSE, FALSE);

            if (StopSearch) {
                return 0;
            }

            if (Score <= RazoringAlpha) {
                return Score;
            }
        }
#endif // RAZORING

#ifdef NULL_MOVE_PRUNING
        if (NonPawnMaterial && Depth > 1 && StaticScore >= Beta) { // Hakkapeliitta
            NullMoveReduction = 3 + Depth / 6;

            MakeNullMove(Board);

#ifdef HASH_PREFETCH
            Prefetch(Board->Hash);
#endif // HASH_PREFETCH

            ++Board->Nodes;

            GiveCheck = IsInCheck(Board, Board->CurrentColor);

            // Zero window search for reduced depth
            TempBestMoves[0] = (MoveItem){ 0, 0, 0 }; // End of move list

            Score = -Search(Board, -Beta, -Beta + 1, Depth - 1 - NullMoveReduction, Ply + 1, TempBestMoves, FALSE, GiveCheck, FALSE, 0);

            UnmakeNullMove(Board);

            if (StopSearch) {
                return 0;
            }

            if (Score >= Beta) {
                if (Score >= INF - MAX_PLY) {
                    return Beta;
                }
                else {
                    return Score;
                }
            }
        }
#endif // NULL_MOVE_PRUNING

#ifdef PROBCUT
        if (Depth >= 5 && Beta < INF - MAX_PLY) { // Xiphos
            BetaCut = Beta + 100;

            GenMoveCount = 0;
            GenerateCaptureMoves(Board, CMH_Pointer, MoveList, &GenMoveCount);

            for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) {
                PrepareNextMove(MoveNumber, MoveList, GenMoveCount);

                if (MoveList[MoveNumber].Move == SkipMove) {
                    continue; // Next move
                }

                if (!(MoveList[MoveNumber].Type & MOVE_CAPTURE)) {
                    continue; // Next move
                }

                if (SEE(Board, MoveList[MoveNumber].Type, MoveList[MoveNumber].Move) < BetaCut - StaticScore) {
                    continue; // Next move
                }

                MakeMove(Board, MoveList[MoveNumber]);

#ifdef HASH_PREFETCH
                Prefetch(Board->Hash);
#endif // HASH_PREFETCH

                if (IsInCheck(Board, CHANGE_COLOR(Board->CurrentColor))) { // Illegal move
                    UnmakeMove(Board);

                    continue; // Next move
                }

                ++Board->Nodes;

                GiveCheck = IsInCheck(Board, Board->CurrentColor);

                // Zero window quiescence search
                Score = -QuiescenceSearch(Board, -BetaCut, -BetaCut + 1, 0, Ply + 1, FALSE, GiveCheck);

                if (Score >= BetaCut) {
                    // Zero window search for reduced depth
                    TempBestMoves[0] = (MoveItem){ 0, 0, 0 }; // End of move list

                    Score = -Search(Board, -BetaCut, -BetaCut + 1, Depth - 4, Ply + 1, TempBestMoves, FALSE, GiveCheck, FALSE, 0);
                }

                UnmakeMove(Board);

                if (StopSearch) {
                    return 0;
                }

                if (Score >= BetaCut) {
                    return Score;
                }
            }
        }
#endif // PROBCUT
    } // if
#endif // REVERSE_FUTILITY_PRUNING || RAZORING || NULL_MOVE_PRUNING || PROBCUT

#ifdef IID
    if (!SkipMove && !HashMove && (IsPrincipal ? Depth > 4 : Depth > 7)) { // Hakkapeliitta
#ifdef DEBUG_IID
        printf("-- IID: Depth = %d\n", Depth);
#endif // DEBUG_IID

        // Search with full window for reduced depth
        TempBestMoves[0] = (MoveItem){ 0, 0, 0 }; // End of move list

        Search(Board, Alpha, Beta, (IsPrincipal ? Depth - 2 : Depth / 2), Ply, TempBestMoves, IsPrincipal, InCheck, FALSE, 0);

        if (StopSearch) {
            return 0;
        }

        LoadHash(Board->Hash, &HashDepth, Ply, &HashScore, &HashStaticScore, &HashMove, &HashFlag);

#ifdef DEBUG_IID
        if (HashMove) {
            printf("-- IID: Depth = %d Move = %s%s\n", Depth, BoardName[MOVE_FROM(HashMove)], BoardName[MOVE_TO(HashMove)]);
        }
#endif // DEBUG_IID
    }
#endif // IID

    BestScore = -INF + Ply;

    GenMoveCount = 0;
    GenerateAllMoves(Board, CMH_Pointer, MoveList, &GenMoveCount);

    SetHashMoveSortValue(MoveList, GenMoveCount, HashMove);

#ifdef KILLER_MOVE
    SetKillerMove1SortValue(Board, Ply, MoveList, GenMoveCount, HashMove);
    SetKillerMove2SortValue(Board, Ply, MoveList, GenMoveCount, HashMove);
#endif // KILLER_MOVE

#ifdef COUNTER_MOVE
    SetCounterMoveSortValue(Board, Ply, MoveList, GenMoveCount, HashMove);
#endif // COUNTER_MOVE

#ifdef KILLER_MOVE
    Board->KillerMoveTable[Ply + 1][0] = 0;
    Board->KillerMoveTable[Ply + 1][1] = 0;
#endif // KILLER_MOVE

    for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) {
#ifdef BAD_CAPTURE_LAST
NextMove:
#endif // BAD_CAPTURE_LAST

        PrepareNextMove(MoveNumber, MoveList, GenMoveCount);

        if (MoveList[MoveNumber].Move == SkipMove) {
            continue; // Next move
        }

#ifdef BAD_CAPTURE_LAST
        if (
            (MoveList[MoveNumber].Type & MOVE_CAPTURE)
            && !(MoveList[MoveNumber].Type & MOVE_PAWN_PROMOTE)
            && MoveList[MoveNumber].SortValue >= 0
            && MoveList[MoveNumber].Move != HashMove
        ) {
            SEE_Value = SEE(Board, MoveList[MoveNumber].Type, MoveList[MoveNumber].Move);

            if (SEE_Value < 0) { // Bad capture move
                MoveList[MoveNumber].SortValue = SEE_Value - SORT_CAPTURE_MOVE_BONUS; // Search move later

                goto NextMove;
            }
        }
#endif // BAD_CAPTURE_LAST

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

#ifdef PRINT_CURRENT_MOVE
        if (omp_get_thread_num() == 0) { // Master thread
            if (Ply == 0 && PrintMode == PRINT_MODE_UCI && (Clock() - TimeStart) >= 3000ULL) {
#pragma omp critical
                {
                    printf("info depth %d currmovenumber %d currmove %s%s", Depth, MoveNumber + 1, BoardName[MOVE_FROM(MoveList[MoveNumber].Move)], BoardName[MOVE_TO(MoveList[MoveNumber].Move)]);

                    if (MoveList[MoveNumber].Type & MOVE_PAWN_PROMOTE) {
                        printf("%c", PiecesCharBlack[MOVE_PROMOTE_PIECE_TYPE(MoveList[MoveNumber].Move)]);
                    }

                    printf("\n");
                }
            }
        }
#endif // PRINT_CURRENT_MOVE

        GiveCheck = IsInCheck(Board, Board->CurrentColor);

        Extension = 0;

#ifdef CHECK_EXTENSION
        if (!Extension && GiveCheck) {
            Extension = 1;
        }
#endif // CHECK_EXTENSION

#ifdef SINGULAR_EXTENSION
        if (
            !Extension
            && !SkipMove
            && Ply > 0
            && Depth >= 8
            && MoveList[MoveNumber].Move == HashMove
            && HashFlag == HASH_BETA
            && HashDepth >= Depth - 3
            && (HashScore > -INF + MAX_PLY && HashScore < INF - MAX_PLY)
        ) { // Xiphos
            UnmakeMove(Board);

            SingularBeta = HashScore - Depth;

            // Zero window search for reduced depth
            TempBestMoves[0] = (MoveItem){ 0, 0, 0 }; // End of move list

            Score = Search(Board, SingularBeta - 1, SingularBeta, Depth / 2, Ply, TempBestMoves, FALSE, InCheck, FALSE, MoveList[MoveNumber].Move);

            if (StopSearch) {
                return 0;
            }

#ifdef DEBUG_SINGULAR_EXTENSION
            PrintBoard(Board);

            printf("-- SE: Ply = %d Depth = %d SkipMove = %s%s HashScore = %d SingularBeta = %d Score = %d\n", Ply, Depth, BoardName[MOVE_FROM(MoveList[MoveNumber].Move)], BoardName[MOVE_TO(MoveList[MoveNumber].Move)], HashScore, SingularBeta, Score);
#endif // DEBUG_SINGULAR_EXTENSION

            if (Score < SingularBeta) {
#ifdef DEBUG_SINGULAR_EXTENSION
                printf("-- SE: Extension = 1\n");
#endif // DEBUG_SINGULAR_EXTENSION

                Extension = 1;
            }

            MakeMove(Board, MoveList[MoveNumber]);
        }
#endif // SINGULAR_EXTENSION

#if defined(COUNTER_MOVE_HISTORY) && defined(COUNTER_MOVE_HISTORY_EXTENSION)
        if (
            !Extension
            && !(MoveList[MoveNumber].Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE)) // Not capture/promote move
            && (CMH_Pointer[0] && CMH_Pointer[0][(PIECE_TYPE(Board->Pieces[MOVE_FROM(MoveList[MoveNumber].Move)]) << 6) + MOVE_TO(MoveList[MoveNumber].Move)] >= MAX_HEURISTIC_SCORE / 2)
            && (CMH_Pointer[1] && CMH_Pointer[1][(PIECE_TYPE(Board->Pieces[MOVE_FROM(MoveList[MoveNumber].Move)]) << 6) + MOVE_TO(MoveList[MoveNumber].Move)] >= MAX_HEURISTIC_SCORE / 2)
        ) { // Xiphos
            Extension = 1;
        }
#endif // COUNTER_MOVE_HISTORY && COUNTER_MOVE_HISTORY_EXTENSION

#if defined(FUTILITY_PRUNING) || defined(LATE_MOVE_PRUNING) || defined(SEE_QUIET_MOVE_PRUNING) || defined(SEE_CAPTURE_MOVE_PRUNING)
        if (
            !Extension
            && !IsPrincipal
            && !InCheck
            && !GiveCheck
            && MoveList[MoveNumber].Move != HashMove
        ) {
#ifdef SEE_CAPTURE_MOVE_PRUNING
            if (Depth <= 3) {
#ifdef BAD_CAPTURE_LAST
                if (MoveList[MoveNumber].SortValue + SORT_CAPTURE_MOVE_BONUS < -100 * Depth) { // Bad capture move (Xiphos)
                    UnmakeMove(Board);

                    continue; // Next move
                }
#else
                UnmakeMove(Board);

                if (SEE(Board, MoveList[MoveNumber].Type, MoveList[MoveNumber].Move) < -100 * Depth) { // Bad capture move (Xiphos)
                    continue; // Next move
                }

                MakeMove(Board, MoveList[MoveNumber]);
#endif // BAD_CAPTURE_LAST
            }
#endif // SEE_CAPTURE_MOVE_PRUNING

#if defined(FUTILITY_PRUNING) || defined(LATE_MOVE_PRUNING) || defined(SEE_QUIET_MOVE_PRUNING)
            if (!(MoveList[MoveNumber].Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE))) { // Not capture/promote move
#ifdef FUTILITY_PRUNING
                if (Depth <= 7 && (StaticScore + FutilityMargin(Depth)) <= Alpha) { // Hakkapeliitta
                    BestScore = MAX(BestScore, StaticScore + FutilityMargin(Depth));

                    UnmakeMove(Board);

                    continue; // Next move
                }
#endif // FUTILITY_PRUNING

#ifdef LATE_MOVE_PRUNING
                if (Depth <= 6 && MoveNumber >= LateMovePruningTable[Depth]) { // Hakkapeliitta
                    UnmakeMove(Board);

                    continue; // Next move
                }
#endif // LATE_MOVE_PRUNING

#ifdef SEE_QUIET_MOVE_PRUNING
                if (Depth <= 3) { // Hakkapeliitta
                    UnmakeMove(Board);

                    if (SEE(Board, MoveList[MoveNumber].Type, MoveList[MoveNumber].Move) < 0) { // Bad quiet move
                        continue; // Next move
                    }

                    MakeMove(Board, MoveList[MoveNumber]);
                }
#endif // SEE_QUIET_MOVE_PRUNING
            }
#endif // FUTILITY_PRUNING || LATE_MOVE_PRUNING || SEE_QUIET_MOVE_PRUNING
        } // if
#endif // FUTILITY_PRUNING || LATE_MOVE_PRUNING || SEE_QUIET_MOVE_PRUNING || SEE_CAPTURE_MOVE_PRUNING

        NewDepth = Depth - 1 + Extension;

        if (IsPrincipal && LegalMoveCount == 1) {
            // Search with full window
            TempBestMoves[0] = (MoveItem){ 0, 0, 0 }; // End of move list

            Score = -Search(Board, -Beta, -Alpha, NewDepth, Ply + 1, TempBestMoves, TRUE, GiveCheck, TRUE, 0);
        }
        else { // !IsPrincipal || LegalMoveCount > 1
#ifdef LATE_MOVE_REDUCTION
            if (
                !Extension
                && !InCheck
                && !GiveCheck
                && !(MoveList[MoveNumber].Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE)) // Not capture/promote move
                && MoveList[MoveNumber].Move != HashMove
                && Depth >= 5
            ) {
                LateMoveReduction = LateMoveReductionTable[MIN(Depth, 63)][MIN(MoveNumber, 63)]; // Hakkapeliitta

                if (IsPrincipal) {
                    --LateMoveReduction;
                }
            }
            else {
                LateMoveReduction = 0;
            }

            // Zero window search for reduced depth
            TempBestMoves[0] = (MoveItem){ 0, 0, 0 }; // End of move list

            Score = -Search(Board, -Alpha - 1, -Alpha, NewDepth - LateMoveReduction, Ply + 1, TempBestMoves, FALSE, GiveCheck, TRUE, 0);

            if (LateMoveReduction > 0 && Score > Alpha) {
#endif // LATE_MOVE_REDUCTION
                // Zero window search
                TempBestMoves[0] = (MoveItem){ 0, 0, 0 }; // End of move list

                Score = -Search(Board, -Alpha - 1, -Alpha, NewDepth, Ply + 1, TempBestMoves, FALSE, GiveCheck, TRUE, 0);
#ifdef LATE_MOVE_REDUCTION
            }
#endif // LATE_MOVE_REDUCTION

            if (IsPrincipal && Score > Alpha && (Ply == 0 || Score < Beta)) {
                // Search with full window
                TempBestMoves[0] = (MoveItem){ 0, 0, 0 }; // End of move list

                Score = -Search(Board, -Beta, -Alpha, NewDepth, Ply + 1, TempBestMoves, TRUE, GiveCheck, TRUE, 0);
            }
        }

        UnmakeMove(Board);

        if (StopSearch) {
            return 0;
        }

        if (Score > BestScore) {
            BestScore = Score;

            if (BestScore > Alpha) {
                BestMove = MoveList[MoveNumber];

                if (IsPrincipal) {
                    if (omp_get_thread_num() == 0) { // Master thread
                        if (Ply == 0) { // Root node
                            if (BestMove.Move == BestMoves[0].Move) { // Move not changed
                                if (TimeStep > 0) {
                                    --TimeStep;
                                }
                            }
                            else { // Move changed
                                TimeStep = MAX_TIME_STEPS - 1;
                            }
                        }
                    }

                    SaveBestMoves(BestMoves, BestMove, TempBestMoves);
                }

                if (IsPrincipal && BestScore < Beta) {
                    Alpha = BestScore;
                }
                else { // !IsPrincipal || BestScore >= Beta
#ifdef USE_STATISTIC
                    ++Board->CutoffCount;
#endif // USE_STATISTIC

                    if (!(BestMove.Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE))) { // Not capture/promote move
                        UpdateHeuristic(Board, CMH_Pointer, BestMove.Move, BONUS(Depth));

                        for (int QuietMoveNumber = 0; QuietMoveNumber < QuietMoveCount; ++QuietMoveNumber) {
                            UpdateHeuristic(Board, CMH_Pointer, QuietMoveList[QuietMoveNumber], -BONUS(Depth));
                        }

#ifdef KILLER_MOVE
                        UpdateKillerMove(Board, BestMove.Move, Ply);
#endif // KILLER_MOVE

#ifdef COUNTER_MOVE
                        UpdateCounterMove(Board, BestMove.Move, Ply);
#endif // COUNTER_MOVE
                    }

                    break;
                }
            } // if
        } // if

        if (!(MoveList[MoveNumber].Type & (MOVE_CAPTURE | MOVE_PAWN_PROMOTE))) { // Not capture/promote move
            QuietMoveList[QuietMoveCount++] = MoveList[MoveNumber].Move;
        }
    } // for

    if (LegalMoveCount == 0) { // No legal moves
        if (SkipMove) {
            BestScore = Alpha;
        }
        else if (InCheck) { // Checkmate
            BestScore = -INF + Ply;
        }
        else { // Stalemate
            BestScore = 0;
        }
    }

    if (!SkipMove) {
        if (BestScore >= Beta) {
            HashFlag = HASH_BETA;
        }
        else if (IsPrincipal && BestMove.Move) {
            HashFlag = HASH_EXACT;
        }
        else {
            HashFlag = HASH_ALPHA;
        }

        SaveHash(Board->Hash, Depth, Ply, BestScore, StaticScore, BestMove.Move, HashFlag);
    }

    return BestScore;
}