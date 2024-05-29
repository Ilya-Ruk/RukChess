// Game.cpp

#include "stdafx.h"

#include "Game.h"

#include "Board.h"
#include "Book.h"
#include "Def.h"
#include "Gen.h"
#include "Hash.h"
#include "Heuristic.h"
#include "Move.h"
#include "Search.h"
#include "Types.h"
#include "Utils.h"

int MaxThreads;
int MaxDepth;
U64 MaxTime;

U64 TimeForMove;

U64 TimeStart;
U64 TimeStop;
U64 TotalTime;

int TimeStep;
U64 TargetTime[MAX_TIME_STEPS];

int CompletedDepth;

volatile BOOL StopSearch;

int PrintMode = PRINT_MODE_NORMAL;

void PrintBestMoves(const BoardItem* Board, const int Depth, const MoveItem* BestMoves, const int BestScore)
{
    U64 CurrentTime = Clock();

    TotalTime = CurrentTime - TimeStart;

    if (PrintMode == PRINT_MODE_UCI) {
        printf("info depth %d seldepth %d nodes %llu time %llu", Depth, Board->SelDepth, Board->Nodes, TotalTime);

        if (BestScore <= -INF + MAX_PLY) {
            printf(" score mate %d", (-INF - BestScore) / 2);
        }
        else if (BestScore >= INF - MAX_PLY) {
            printf(" score mate %d", (INF - BestScore + 1) / 2);
        }
        else {
            printf(" score cp %d", BestScore);
        }

        if (TotalTime > 1000ULL) {
            printf(" nps %llu", 1000ULL * Board->Nodes / TotalTime);
        }

        if ((TimeStop - CurrentTime) >= 3000ULL) {
            printf(" hashfull %d", FullHash());
        }

        printf(" pv");
    }
    else { // PRINT_MODE_NORMAL/PRINT_MODE_TESTS
        if (Depth == 1) {
            printf("\n");
        }

        printf("Depth %2d / %2d Score %6.2f Nodes %9llu Hashfull %5.2f%% Time %6.2f", Depth, Board->SelDepth, (double)BestScore / 100.0, Board->Nodes, (double)FullHash() / 10.0, (double)TotalTime / 1000.0);

        printf(" PV");
    }

    for (int Ply = 0; Ply < MAX_PLY && BestMoves[Ply].Move; ++Ply) {
        printf(" %s%s", BoardName[MOVE_FROM(BestMoves[Ply].Move)], BoardName[MOVE_TO(BestMoves[Ply].Move)]);

        if (BestMoves[Ply].Type & MOVE_PAWN_PROMOTE) {
            printf("%c", PiecesCharBlack[MOVE_PROMOTE_PIECE(BestMoves[Ply].Move)]);
        }
    }

    printf("\n");
}

void SaveBestMoves(MoveItem* BestMoves, const MoveItem BestMove, const MoveItem* TempBestMoves)
{
    BestMoves[0] = BestMove;

    for (int Ply = 1; Ply < MAX_PLY; ++Ply) {
        BestMoves[Ply] = TempBestMoves[Ply - 1];

        if (!BestMoves[Ply].Move) {
            break;
        }
    }
}

BOOL PrintResult(const BOOL InCheck, const MoveItem BestMove, const MoveItem PonderMove, const int BestScore)
{
    char NotateMoveStr[16];

    if (PrintMode == PRINT_MODE_UCI) {
        printf("info nodes %llu", CurrentBoard.Nodes);

        if (TotalTime > 1000ULL) {
            printf(" nps %llu", 1000ULL * CurrentBoard.Nodes / TotalTime);
        }

        if ((TimeStop - Clock()) >= 3000ULL) {
            printf(" hashfull %d", FullHash());
        }

        printf("\n");

        if (BestMove.Move) {
            printf("bestmove %s%s", BoardName[MOVE_FROM(BestMove.Move)], BoardName[MOVE_TO(BestMove.Move)]);

            if (BestMove.Type & MOVE_PAWN_PROMOTE) {
                printf("%c", PiecesCharBlack[MOVE_PROMOTE_PIECE(BestMove.Move)]);
            }

            if (PonderMove.Move) {
                printf(" ponder %s%s", BoardName[MOVE_FROM(PonderMove.Move)], BoardName[MOVE_TO(PonderMove.Move)]);

                if (PonderMove.Type & MOVE_PAWN_PROMOTE) {
                    printf("%c", PiecesCharBlack[MOVE_PROMOTE_PIECE(PonderMove.Move)]);
                }
            }
        }
        else { // No legal moves
            printf("bestmove (none)");
        }

        printf("\n");

        return FALSE;
    }
    else if (PrintMode == PRINT_MODE_NORMAL) {
        if (!BestMove.Move) { // No legal moves
            if (InCheck) { // Checkmate
                printf("\n");

                printf("Checkmate!\n");

                printf("\n");

                if (CurrentBoard.CurrentColor == WHITE) {
                    printf("{0-1} Black wins!\n");
                }
                else { // BLACK
                    printf("{1-0} White wins!\n");
                }
            }
            else { // Stalemate
                printf("\n");

                printf("{1/2-1/2} Stalemate!\n");
            }

            return FALSE;
        }

        NotateMove(&CurrentBoard, BestMove, NotateMoveStr);

        MakeMove(&CurrentBoard, BestMove);

        PrintBoard(&CurrentBoard);

        printf("\n");

        if (CurrentBoard.CurrentColor == WHITE) {
            printf("%d: ... %s%s", CurrentBoard.HalfMoveNumber / 2, BoardName[MOVE_FROM(BestMove.Move)], BoardName[MOVE_TO(BestMove.Move)]);
        }
        else { // BLACK
            printf("%d: %s%s", CurrentBoard.HalfMoveNumber / 2 + 1, BoardName[MOVE_FROM(BestMove.Move)], BoardName[MOVE_TO(BestMove.Move)]);
        }

        if (BestMove.Type & MOVE_PAWN_PROMOTE) {
            printf("%c", PiecesCharBlack[MOVE_PROMOTE_PIECE(BestMove.Move)]);
        }

        printf(" (%s)\n", NotateMoveStr);

        printf("\n");

        printf("Score %.2f Nodes %llu Hashfull %.2f%% Time %.2f", (double)BestScore / 100.0, CurrentBoard.Nodes, (double)FullHash() / 10.0, (double)TotalTime / 1000.0);

        if (TotalTime > 1000ULL) {
            printf(" NPS %llu", 1000ULL * CurrentBoard.Nodes / TotalTime);
        }

        printf("\n");

#ifdef DEBUG_STATISTIC
        printf("\n");

        printf("Hash count %llu Evaluate count %llu Cutoff count %llu Quiescence count %llu\n", CurrentBoard.HashCount, CurrentBoard.EvaluateCount, CurrentBoard.CutoffCount, CurrentBoard.QuiescenceCount);
#endif // DEBUG_STATISTIC

        if (BestScore <= -INF + 1 || BestScore >= INF - 1) { // Checkmate
            printf("\n");

            printf("Checkmate!\n");

            printf("\n");

            if (CurrentBoard.CurrentColor == WHITE) {
                printf("{0-1} Black wins!\n");
            }
            else { // BLACK
                printf("{1-0} White wins!\n");
            }

            return FALSE;
        }

        if (IsInsufficientMaterial(&CurrentBoard)) {
            printf("\n");

            printf("{1/2-1/2} Draw by insufficient material!\n");

            return FALSE;
        }

        if (CurrentBoard.FiftyMove >= 100) {
            printf("\n");

            printf("{1/2-1/2} Draw by fifty move rule!\n");

            return FALSE;
        }

        if (PositionRepeat2(&CurrentBoard) == 2) {
            printf("\n");

            printf("{1/2-1/2} Draw by repetition!\n");

            return FALSE;
        }

        return TRUE;
    }
    else { // PRINT_MODE_TESTS
        return FALSE;
    }
}

BOOL ComputerMove(void)
{
    BOOL InCheck;

    int Score = 0;
    int BestScore = 0;

    BoardItem ThreadBoard;
    int ThreadScore;

    volatile int ThreadDepth[MAX_PLY];

    int SearchDepthCount;

    MoveItem BestMove = (MoveItem){ 0, 0, 0 };
    MoveItem PonderMove = (MoveItem){ 0, 0, 0 };

#ifdef ASPIRATION_WINDOW
    int Delta;

    int Alpha;
    int Beta;
#endif // ASPIRATION_WINDOW

    U64 TargetTimeLocal;

    TimeStart = Clock();
    TimeStop = TimeStart + MaxTime;

    TimeStep = 0;

    CompletedDepth = 0;

    StopSearch = FALSE;

    InCheck = IsInCheck(&CurrentBoard, CurrentBoard.CurrentColor);

    CurrentBoard.Nodes = 0ULL;

#ifdef DEBUG_STATISTIC
    CurrentBoard.HashCount = 0ULL;
    CurrentBoard.EvaluateCount = 0ULL;
    CurrentBoard.CutoffCount = 0ULL;
    CurrentBoard.QuiescenceCount = 0ULL;
#endif // DEBUG_STATISTIC

    CurrentBoard.SelDepth = 0;

    CurrentBoard.BestMovesRoot[0] = (MoveItem){ 0, 0, 0 };

    ClearHeuristic(&CurrentBoard);

#ifdef KILLER_MOVE
    ClearKillerMove(&CurrentBoard);
#endif // KILLER_MOVE

#ifdef COUNTER_MOVE
    ClearCounterMove(&CurrentBoard);
#endif // COUNTER_MOVE

    AddHashStoreIteration();

    if (BookFileLoaded && GetBookMove(&CurrentBoard, CurrentBoard.BestMovesRoot)) {
        BestMove = CurrentBoard.BestMovesRoot[0];
        PonderMove = CurrentBoard.BestMovesRoot[1];

        goto Done;
    }

    for (int Depth = 0; Depth < MAX_PLY; ++Depth) {
        ThreadDepth[Depth] = 0;
    }

#ifdef ASPIRATION_WINDOW
#pragma omp parallel private(Alpha, Beta, Delta, SearchDepthCount, ThreadBoard, ThreadScore)
#else
#pragma omp parallel private(SearchDepthCount, ThreadBoard, ThreadScore)
#endif // ASPIRATION_WINDOW
    {
#if defined(BIND_THREAD) || defined(BIND_THREAD_V2)
        BindThread(omp_get_thread_num());
#endif // BIND_THREAD || BIND_THREAD_V2

        ThreadBoard = CurrentBoard;
        ThreadScore = Score;

        for (int Depth = 1; Depth <= MaxDepth; ++Depth) {
#pragma omp critical
            {
                SearchDepthCount = ++ThreadDepth[Depth - 1];
            }

            if (omp_get_thread_num() > 0) { // Helper thread
                if (Depth > 1 && Depth < MaxDepth && SearchDepthCount > MAX((omp_get_max_threads() + 1) / 2, 2)) {
                    continue; // Next depth
                }
            }
/*
#pragma omp critical
            {
                printf("-- Start: Depth = %d Thread number = %d\n", Depth, omp_get_thread_num());
            }
*/
            ThreadBoard.Nodes = 0ULL;

#ifdef DEBUG_STATISTIC
            ThreadBoard.HashCount = 0ULL;
            ThreadBoard.EvaluateCount = 0ULL;
            ThreadBoard.CutoffCount = 0ULL;
            ThreadBoard.QuiescenceCount = 0ULL;
#endif // DEBUG_STATISTIC

            ThreadBoard.SelDepth = 0;

#ifdef ASPIRATION_WINDOW
            if (Depth >= 4) {
                Delta = ASPIRATION_WINDOW_INIT_DELTA;

                Alpha = MAX((ThreadScore - Delta), -INF);
                Beta = MIN((ThreadScore + Delta), INF);

                while (TRUE) {
                    ThreadScore = Search(&ThreadBoard, Alpha, Beta, Depth, 0, ThreadBoard.BestMovesRoot, TRUE, InCheck, FALSE, 0);

                    if (StopSearch) {
                        break; // while
                    }

                    if (ThreadScore <= Alpha) {
                        Alpha = MAX((ThreadScore - Delta), -INF);
                    }
                    else if (ThreadScore >= Beta) {
                        Beta = MIN((ThreadScore + Delta), INF);
                    }
                    else {
                        break; // while
                    }

                    Delta += Delta / 4 + 5;
                }
            }
            else {
#endif // ASPIRATION_WINDOW
                ThreadScore = Search(&ThreadBoard, -INF, INF, Depth, 0, ThreadBoard.BestMovesRoot, TRUE, InCheck, FALSE, 0);
#ifdef ASPIRATION_WINDOW
            }
#endif // ASPIRATION_WINDOW

#pragma omp critical
            {
//                printf("-- End: Depth = %d Thread number = %d\n", Depth, omp_get_thread_num());

                CurrentBoard.Nodes += ThreadBoard.Nodes;

#ifdef DEBUG_STATISTIC
                CurrentBoard.HashCount += ThreadBoard.HashCount;
                CurrentBoard.EvaluateCount += ThreadBoard.EvaluateCount;
                CurrentBoard.CutoffCount += ThreadBoard.CutoffCount;
                CurrentBoard.QuiescenceCount += ThreadBoard.QuiescenceCount;
#endif // DEBUG_STATISTIC

                CurrentBoard.SelDepth = MAX(CurrentBoard.SelDepth, ThreadBoard.SelDepth);
            }

            if (StopSearch) {
                break; // for (depth)
            }

            if (omp_get_thread_num() == 0) { // Master thread
#pragma omp critical
                {
                    Score = ThreadScore;

                    for (int Ply = 0; Ply < MAX_PLY; ++Ply) {
                        CurrentBoard.BestMovesRoot[Ply] = ThreadBoard.BestMovesRoot[Ply];

                        if (!CurrentBoard.BestMovesRoot[Ply].Move) {
                            break; // for (ply)
                        }
                    }

                    CompletedDepth = Depth;

                    PrintBestMoves(&CurrentBoard, CompletedDepth, CurrentBoard.BestMovesRoot, Score);

                    BestMove = CurrentBoard.BestMovesRoot[0];
                    PonderMove = CurrentBoard.BestMovesRoot[1];

                    TargetTimeLocal = TargetTime[TimeStep];

                    if (TargetTimeLocal > 0ULL && BestScore > Score) {
                        TargetTimeLocal = (U64)((double)TargetTimeLocal * MIN((1.0 + (double)(BestScore - Score) / 80.0), 2.0));
                    }

                    BestScore = Score;

                    if (Depth == MaxDepth) { // Stop helper threads
                        StopSearch = TRUE;
                    }

                    if (!BestMove.Move) { // No legal moves
                        StopSearch = TRUE;
                    }

                    if (BestScore <= -INF + Depth || BestScore >= INF - Depth) { // Checkmate
                        StopSearch = TRUE;
                    }

                    if (TargetTimeLocal > 0ULL && CompletedDepth >= MIN_SEARCH_DEPTH && (Clock() - TimeStart) >= TargetTimeLocal) {
                        StopSearch = TRUE;
                    }
                } // pragma omp critical
            } // if

            if (StopSearch) {
                break; // for (depth)
            }
        } // for
    } // pragma omp parallel

Done:

    TimeStop = Clock();
    TotalTime = TimeStop - TimeStart;

    return PrintResult(InCheck, BestMove, PonderMove, BestScore);
}

void ComputerMoveThread(void* ignored)
{
    ComputerMove();

    _endthread();
}

BOOL HumanMove(void)
{
    char ReadStr[10];

    int File;
    int Rank;

    int From;
    int To;
    int PromotePiece;

    int Move;

    char NotateMoveStr[16];

    int GenMoveCount;
    MoveItem MoveList[MAX_GEN_MOVES];

    BOOL InCheck = IsInCheck(&CurrentBoard, CurrentBoard.CurrentColor);

    GenMoveCount = 0;
    GenerateAllMoves(&CurrentBoard, NULL, MoveList, &GenMoveCount);

    while (TRUE) {
        ReadStr[0] = '\0'; // Nul

        printf("\n");

        printf("Enter move (e2e4, e7e8q, undo, save, exit) %c ", (InCheck ? '!' : '>'));
        scanf_s("%9s", ReadStr, (unsigned)_countof(ReadStr));

        if (
            !strcmp(ReadStr, "undo")
            && CurrentBoard.HalfMoveNumber >= 2
            && CurrentBoard.MoveTable[CurrentBoard.HalfMoveNumber - 1].Hash
            && CurrentBoard.MoveTable[CurrentBoard.HalfMoveNumber - 2].Hash
        ) {
            UnmakeMove(&CurrentBoard);
            UnmakeMove(&CurrentBoard);

            PrintBoard(&CurrentBoard);

            InCheck = IsInCheck(&CurrentBoard, CurrentBoard.CurrentColor);

            GenMoveCount = 0;
            GenerateAllMoves(&CurrentBoard, NULL, MoveList, &GenMoveCount);

            continue; // Next string
        }

        if (!strcmp(ReadStr, "save")) {
            SaveGame(&CurrentBoard);

            continue; // Next string
        }

        if (!strcmp(ReadStr, "exit")) {
            return FALSE;
        }

        if (strlen(ReadStr) == 4 || strlen(ReadStr) == 5) { // Move (e2e4, e7e8q)
            File = ReadStr[0] - 'a';
            Rank = 7 - (ReadStr[1] - '1');

            From = SQUARE(Rank, File);

            File = ReadStr[2] - 'a';
            Rank = 7 - (ReadStr[3] - '1');

            To = SQUARE(Rank, File);

            if (ReadStr[4] == 'Q' || ReadStr[4] == 'q') {
                PromotePiece = QUEEN;
            }
            else if (ReadStr[4] == 'R' || ReadStr[4] == 'r') {
                PromotePiece = ROOK;
            }
            else if (ReadStr[4] == 'B' || ReadStr[4] == 'b') {
                PromotePiece = BISHOP;
            }
            else if (ReadStr[4] == 'N' || ReadStr[4] == 'n') {
                PromotePiece = KNIGHT;
            }
            else {
                PromotePiece = 0;
            }

            Move = MOVE_CREATE(From, To, PromotePiece);

            for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) {
                if (MoveList[MoveNumber].Move == Move) {
                    NotateMove(&CurrentBoard, MoveList[MoveNumber], NotateMoveStr);

                    MakeMove(&CurrentBoard, MoveList[MoveNumber]);

                    if (IsInCheck(&CurrentBoard, CHANGE_COLOR(CurrentBoard.CurrentColor))) { // Illegal move
                        UnmakeMove(&CurrentBoard);

                        break; // for
                    }

                    PrintBoard(&CurrentBoard);

                    printf("\n");

                    if (CurrentBoard.CurrentColor == WHITE) {
                        printf("%d: ... %s%s", CurrentBoard.HalfMoveNumber / 2, BoardName[MOVE_FROM(MoveList[MoveNumber].Move)], BoardName[MOVE_TO(MoveList[MoveNumber].Move)]);
                    }
                    else { // BLACK
                        printf("%d: %s%s", CurrentBoard.HalfMoveNumber / 2 + 1, BoardName[MOVE_FROM(MoveList[MoveNumber].Move)], BoardName[MOVE_TO(MoveList[MoveNumber].Move)]);
                    }

                    if (MoveList[MoveNumber].Type & MOVE_PAWN_PROMOTE) {
                        printf("%c", PiecesCharBlack[MOVE_PROMOTE_PIECE(MoveList[MoveNumber].Move)]);
                    }

                    printf(" (%s)\n", NotateMoveStr);

                    if (IsInsufficientMaterial(&CurrentBoard)) {
                        printf("\n");

                        printf("{1/2-1/2} Draw by insufficient material!\n");

                        return FALSE;
                    }

                    if (CurrentBoard.FiftyMove >= 100) {
                        printf("\n");

                        printf("{1/2-1/2} Draw by fifty move rule!\n");

                        return FALSE;
                    }

                    if (PositionRepeat2(&CurrentBoard) == 2) {
                        printf("\n");

                        printf("{1/2-1/2} Draw by repetition!\n");

                        return FALSE;
                    }

                    return TRUE;
                } // if
            } // for
        } // if
    } // while
}

void InputParametrs(void)
{
    int InputDepth;
    int InputMaxTime;
    int InputHashSize;
    int InputThreads;

    printf("\n");

    printf("Max. depth (min. 1 max. %d; 0 = %d): ", MAX_PLY, MAX_PLY);
    scanf_s("%d", &InputDepth);

    MaxDepth = (InputDepth >= 1 && InputDepth <= MAX_PLY) ? InputDepth : MAX_PLY;

    printf("Max. time (min. 1 max. %d; 0 = %d), sec.: ", MAX_TIME, MAX_TIME);
    scanf_s("%d", &InputMaxTime);

    MaxTime = (InputMaxTime >= 1 && InputMaxTime <= MAX_TIME) ? (U64)InputMaxTime : (U64)MAX_TIME;
    MaxTime *= 1000ULL;
    MaxTime -= (U64)REDUCE_TIME;

    TimeForMove = 0ULL;

    memset(TargetTime, 0, sizeof(TargetTime));

    printf("Hash table size (min. 1 max. %d; 0 = %d), Mb: ", MAX_HASH_TABLE_SIZE, DEFAULT_HASH_TABLE_SIZE);
    scanf_s("%d", &InputHashSize);

    InputHashSize = (InputHashSize >= 1 && InputHashSize <= MAX_HASH_TABLE_SIZE) ? InputHashSize : DEFAULT_HASH_TABLE_SIZE;

    InitHashTable(InputHashSize);
    ClearHash();

    printf("Threads (min. 1 max. %d; 0 = %d): ", MaxThreads, DEFAULT_THREADS);
    scanf_s("%d", &InputThreads);

    InputThreads = (InputThreads >= 1 && InputThreads <= MaxThreads) ? InputThreads : DEFAULT_THREADS;

    omp_set_num_threads(InputThreads);
}

void Game(const int HumanColor, const int ComputerColor)
{
    InputParametrs();

    PrintBoard(&CurrentBoard);

    while (TRUE) {
        if (CurrentBoard.CurrentColor == HumanColor) {
            if (!HumanMove()) {
                return;
            }
        }

        if (CurrentBoard.CurrentColor == ComputerColor) {
            if (!ComputerMove()) {
                return;
            }
        }
    }
}

void GameAuto(void)
{
    InputParametrs();

    PrintBoard(&CurrentBoard);

    while (TRUE) {
        if (!ComputerMove()) {
            return;
        }
    }
}

void LoadGame(BoardItem* Board)
{
    FILE* File;

    char Fen[MAX_FEN_LENGTH];

    fopen_s(&File, "chess.fen", "r");

    if (File == NULL) { // File open error
        printf("File 'chess.fen' open error!\n");

        Sleep(3000);

        exit(0);
    }

    fgets(Fen, sizeof(Fen), File);

    SetFen(Board, Fen);

    fclose(File);
}

void SaveGame(const BoardItem* Board)
{
    FILE* File;

    char Fen[MAX_FEN_LENGTH];

    fopen_s(&File, "chess.fen", "w");

    if (File == NULL) { // File create (open) error
        printf("File 'chess.fen' create (open) error!\n");

        Sleep(3000);

        exit(0);
    }

    GetFen(Board, Fen);

    fprintf(File, "%s", Fen);

    fclose(File);
}