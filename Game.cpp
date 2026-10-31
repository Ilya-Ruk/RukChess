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
U64 MaxNodes;
U64 MaxTime;

U64 ReduceTime = (U64)DEFAULT_REDUCE_TIME;

U64 TimeForMove;

U64 TimeStart;
U64 TimeStop;
U64 TotalTime;

int TimeStep;
U64 TargetTime[MAX_TIME_STEPS];

int CompletedDepth;

volatile BOOL StopSearch;

int PrintMode = PRINT_MODE_NORMAL;

void PrintBestMoves(const int Depth, const MoveItem* BestMoves, const int BestScore)
{
    U64 CurrentTime = Clock();

    U64 Nodes = 0ULL;

    int SelDepth = 0;

    for (int Thread = 0; Thread < omp_get_max_threads(); ++Thread) {
        Nodes += ThreadSearchDataList[Thread].Nodes;

        SelDepth = MAX(SelDepth, ThreadSearchDataList[Thread].SelDepth);
    }

    TotalTime = CurrentTime - TimeStart;

    if (PrintMode == PRINT_MODE_UCI) {
        printf("info depth %d seldepth %d nodes %llu time %llu", Depth, SelDepth, Nodes, TotalTime);

        if (BestScore <= -INF + MAX_PLY) {
            printf(" score mate %d", (-INF - BestScore) / 2);
        }
        else if (BestScore >= INF - MAX_PLY) {
            printf(" score mate %d", (INF - BestScore + 1) / 2);
        }
        else {
            printf(" score cp %d", BestScore);
        }

        if (TotalTime >= 1000ULL) {
            printf(" nps %llu", 1000ULL * Nodes / TotalTime);
        }

        if (TimeStop == 0ULL || (TimeStop - CurrentTime) >= 3000ULL) {
            printf(" hashfull %d", FullHash());
        }

        printf(" pv");
    }
    else { // PRINT_MODE_NORMAL/PRINT_MODE_TESTS
        if (Depth == 1) {
            printf("\n");
        }

        printf("Depth %2d / %2d Score %6.2f Nodes %9llu Hashfull %5.2f%% Time %6.2f", Depth, SelDepth, (double)BestScore / 100.0, Nodes, (double)FullHash() / 10.0, (double)TotalTime / 1000.0);

        printf(" PV");
    }

    for (int Ply = 0; Ply < MAX_PLY && BestMoves[Ply].Move; ++Ply) {
        printf(" %s%s", BoardName[MOVE_FROM(BestMoves[Ply].Move)], BoardName[MOVE_TO(BestMoves[Ply].Move)]);

        if (BestMoves[Ply].Type & MOVE_PAWN_PROMOTE) {
            printf("%c", PiecesCharBlack[MOVE_PROMOTE_PIECE_TYPE(BestMoves[Ply].Move)]);
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

    U64 Nodes = 0ULL;

#ifdef USE_STATISTIC
    U64 HashCount = 0ULL;
    U64 EvaluateCount = 0ULL;
    U64 CutoffCount = 0ULL;
    U64 QuiescenceCount = 0ULL;
#endif // USE_STATISTIC

    for (int Thread = 0; Thread < omp_get_max_threads(); ++Thread) {
        Nodes += ThreadSearchDataList[Thread].Nodes;

#ifdef USE_STATISTIC
        HashCount += ThreadSearchDataList[Thread].HashCount;
        EvaluateCount += ThreadSearchDataList[Thread].EvaluateCount;
        CutoffCount += ThreadSearchDataList[Thread].CutoffCount;
        QuiescenceCount += ThreadSearchDataList[Thread].QuiescenceCount;
#endif // USE_STATISTIC
    }

    if (PrintMode == PRINT_MODE_UCI) {
        printf("info nodes %llu", Nodes);

        if (TotalTime >= 1000ULL) {
            printf(" nps %llu", 1000ULL * Nodes / TotalTime);
        }

        if (TimeStop == 0ULL || (TimeStop - Clock()) >= 3000ULL) {
            printf(" hashfull %d", FullHash());
        }

        printf("\n");

        if (BestMove.Move) {
            printf("bestmove %s%s", BoardName[MOVE_FROM(BestMove.Move)], BoardName[MOVE_TO(BestMove.Move)]);

            if (BestMove.Type & MOVE_PAWN_PROMOTE) {
                printf("%c", PiecesCharBlack[MOVE_PROMOTE_PIECE_TYPE(BestMove.Move)]);
            }

            if (PonderMove.Move) {
                printf(" ponder %s%s", BoardName[MOVE_FROM(PonderMove.Move)], BoardName[MOVE_TO(PonderMove.Move)]);

                if (PonderMove.Type & MOVE_PAWN_PROMOTE) {
                    printf("%c", PiecesCharBlack[MOVE_PROMOTE_PIECE_TYPE(PonderMove.Move)]);
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

                if (ThreadBoardList[0].CurrentColor == WHITE) {
                    printf("0-1 {Black mates}!\n");
                }
                else { // BLACK
                    printf("1-0 {White mates}!\n");
                }
            }
            else { // Stalemate
                printf("\n");

                printf("1/2-1/2 {Draw by stalemate}!\n");
            }

            return FALSE;
        }

        NotateMove(&ThreadBoardList[0], BestMove, NotateMoveStr);

        MakeMove(&ThreadBoardList[0], BestMove);

        PrintBoard(&ThreadBoardList[0]);

        printf("\n");

        if (ThreadBoardList[0].CurrentColor == WHITE) {
            printf("%d: ... %s%s", ThreadBoardList[0].HalfMoveNumber / 2, BoardName[MOVE_FROM(BestMove.Move)], BoardName[MOVE_TO(BestMove.Move)]);
        }
        else { // BLACK
            printf("%d: %s%s", ThreadBoardList[0].HalfMoveNumber / 2 + 1, BoardName[MOVE_FROM(BestMove.Move)], BoardName[MOVE_TO(BestMove.Move)]);
        }

        if (BestMove.Type & MOVE_PAWN_PROMOTE) {
            printf("%c", PiecesCharBlack[MOVE_PROMOTE_PIECE_TYPE(BestMove.Move)]);
        }

        printf(" (%s)\n", NotateMoveStr);

        printf("\n");

        printf("Score %.2f Nodes %llu Hashfull %.2f%% Time %.2f", (double)BestScore / 100.0, Nodes, (double)FullHash() / 10.0, (double)TotalTime / 1000.0);

        if (TotalTime >= 1000ULL) {
            printf(" NPS %llu", 1000ULL * Nodes / TotalTime);
        }

        printf("\n");

#ifdef USE_STATISTIC
        printf("\n");

        printf("Hash count %llu Evaluate count %llu Cutoff count %llu Quiescence count %llu\n", HashCount, EvaluateCount, CutoffCount, QuiescenceCount);
#endif // USE_STATISTIC

        if (BestScore <= -INF + 1 || BestScore >= INF - 1) { // Checkmate
            printf("\n");

            if (ThreadBoardList[0].CurrentColor == WHITE) {
                printf("0-1 {Black mates}!\n");
            }
            else { // BLACK
                printf("1-0 {White mates}!\n");
            }

            return FALSE;
        }

        if (IsInsufficientMaterial(&ThreadBoardList[0])) {
            printf("\n");

            printf("1/2-1/2 {Draw by insufficient mating material}!\n");

            return FALSE;
        }

        if (ThreadBoardList[0].FiftyMove >= 100) {
            printf("\n");

            printf("1/2-1/2 {Draw by fifty moves rule}!\n");

            return FALSE;
        }

        if (PositionRepeat2(&ThreadBoardList[0]) == 2) {
            printf("\n");

            printf("1/2-1/2 {Draw by 3-fold repetition}!\n");

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

    int ThreadId;

    BoardItem* ThreadBoard;
    SearchDataItem* ThreadSearchData;

    int ThreadScore;

#ifdef ASPIRATION_WINDOW
    int Alpha;
    int Beta;

    int Delta;
#endif // ASPIRATION_WINDOW

    U64 TargetTimeLocal;

    int BestScore = 0;

    MoveItem BestMove = (MoveItem){ 0, 0, 0 };
    MoveItem PonderMove = (MoveItem){ 0, 0, 0 };

    TimeStart = Clock();

    if (MaxTime > 0ULL) {
        TimeStop = TimeStart + MaxTime;
    }
    else {
        TimeStop = 0ULL;
    }

    TimeStep = 0;

    CompletedDepth = 0;

    StopSearch = FALSE;

    InCheck = IsInCheck(&ThreadBoardList[0], ThreadBoardList[0].CurrentColor);

    AddHashStoreIteration();

    if (GetBookMove(&ThreadBoardList[0], &BestMove)) {
        goto Done;
    }

    for (int Thread = 1; Thread < omp_get_max_threads(); ++Thread) {
        ThreadBoardList[Thread] = ThreadBoardList[0];
    }

#ifdef ASPIRATION_WINDOW
#pragma omp parallel private(ThreadId, ThreadBoard, ThreadSearchData, ThreadScore, Alpha, Beta, Delta, TargetTimeLocal)
#else
#pragma omp parallel private(ThreadId, ThreadBoard, ThreadSearchData, ThreadScore, TargetTimeLocal)
#endif // ASPIRATION_WINDOW
    {
        ThreadId = omp_get_thread_num();

#if defined(BIND_THREAD_V1) || defined(BIND_THREAD_V2)
        BindThread(ThreadId);
#endif // BIND_THREAD_V1 || BIND_THREAD_V2

        ThreadBoard = &ThreadBoardList[ThreadId];
        ThreadSearchData = &ThreadSearchDataList[ThreadId];

        ThreadSearchData->Nodes = 0ULL;

#ifdef USE_STATISTIC
        ThreadSearchData->HashCount = 0ULL;
        ThreadSearchData->EvaluateCount = 0ULL;
        ThreadSearchData->CutoffCount = 0ULL;
        ThreadSearchData->QuiescenceCount = 0ULL;
#endif // USE_STATISTIC

        ThreadSearchData->SelDepth = 0;

        ThreadSearchData->BestMoves[0] = (MoveItem){ 0, 0, 0 }; // End of move list

        ClearHeuristic(ThreadSearchData);

#ifdef KILLER_MOVE
        ClearKillerMove(ThreadSearchData);
#endif // KILLER_MOVE

#ifdef COUNTER_MOVE
        ClearCounterMove(ThreadSearchData);
#endif // COUNTER_MOVE

        ThreadScore = 0;

        for (int Depth = 1; Depth <= MaxDepth; ++Depth) {
#ifdef ASPIRATION_WINDOW
            if (Depth >= ASPIRATION_WINDOW_START_DEPTH) {
                Delta = ASPIRATION_WINDOW_INIT_DELTA;

                Alpha = MAX((ThreadScore - Delta), -INF);
                Beta = MIN((ThreadScore + Delta), INF);

                while (Delta <= INF) {
                    ThreadScore = Search(ThreadBoard, ThreadSearchData, Alpha, Beta, Depth, 0, ThreadSearchData->BestMoves, TRUE, InCheck, FALSE, 0);

                    if (StopSearch) {
                        break; // while
                    }

                    Delta += Delta / 4 + 5;

                    if (ThreadScore <= Alpha) {
                        Beta = (Alpha + Beta) / 2;

                        Alpha = MAX((ThreadScore - Delta), -INF);
                    }
                    else if (ThreadScore >= Beta) {
                        Beta = MIN((ThreadScore + Delta), INF);
                    }
                    else { // ThreadScore > Alpha && ThreadScore < Beta
                        break; // while
                    }
                }
            }
            else {
#endif // ASPIRATION_WINDOW
                ThreadScore = Search(ThreadBoard, ThreadSearchData, -INF, INF, Depth, 0, ThreadSearchData->BestMoves, TRUE, InCheck, FALSE, 0);
#ifdef ASPIRATION_WINDOW
            }
#endif // ASPIRATION_WINDOW

            if (StopSearch) {
                break; // for (depth)
            }

            if (ThreadId == 0) { // Master thread
                CompletedDepth = Depth;

                PrintBestMoves(CompletedDepth, ThreadSearchData->BestMoves, ThreadScore);

                TargetTimeLocal = TargetTime[TimeStep];

                if (TargetTimeLocal > 0ULL && BestScore > ThreadScore) {
                    TargetTimeLocal = (U64)((double)TargetTimeLocal * MIN((1.0 + (double)(BestScore - ThreadScore) / 80.0), 2.0));
                }

                BestScore = ThreadScore;

                if (TargetTimeLocal > 0ULL && CompletedDepth >= MIN_SEARCH_DEPTH && (Clock() - TimeStart) >= TargetTimeLocal) { // Time is up
                    break; // for (depth)
                }
            }

            if (!ThreadSearchData->BestMoves[0].Move) { // No legal moves
                break; // for (depth)
            }

            if (ThreadScore <= -INF + Depth || ThreadScore >= INF - Depth) { // Checkmate
                break; // for (depth)
            }

            if (MaxNodes > 0ULL) {
                U64 Nodes = 0ULL;

                for (int Thread = 0; Thread < omp_get_max_threads(); ++Thread) {
                    Nodes += ThreadSearchDataList[Thread].Nodes;
                }

                if (Nodes >= MaxNodes) {
                    break; // for (depth)
                }
            }
        } // for

        if (ThreadId == 0) { // Master thread
            StopSearch = TRUE; // Stop helper threads
        }
    } // pragma omp parallel

    BestMove = ThreadSearchDataList[0].BestMoves[0];
    PonderMove = ThreadSearchDataList[0].BestMoves[1];

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
    char ReadStr[64];

    int File;
    int Rank;

    int From;
    int To;
    int PromotePieceType;

    int Move;

    char NotateMoveStr[16];

    int GenMoveCount;
    MoveItem MoveList[MAX_GEN_MOVES];

    BOOL InCheck = IsInCheck(&ThreadBoardList[0], ThreadBoardList[0].CurrentColor);

    GenMoveCount = 0;
    GenerateAllMoves(&ThreadBoardList[0], NULL, NULL, MoveList, &GenMoveCount);

    while (TRUE) {
        ReadStr[0] = '\0'; // Nul

        printf("\n");

        printf("Enter move (e2e4, e7e8q, undo, save, exit) %c ", (InCheck ? '!' : '>'));
        scanf_s("%9s", ReadStr, (unsigned)_countof(ReadStr));

        if (
            strcmp(ReadStr, "undo") == 0
            && ThreadBoardList[0].HalfMoveNumber >= 2
            && ThreadBoardList[0].MoveTable[ThreadBoardList[0].HalfMoveNumber - 1].Hash
            && ThreadBoardList[0].MoveTable[ThreadBoardList[0].HalfMoveNumber - 2].Hash
        ) {
            UnmakeMove(&ThreadBoardList[0]);
            UnmakeMove(&ThreadBoardList[0]);

            PrintBoard(&ThreadBoardList[0]);

            InCheck = IsInCheck(&ThreadBoardList[0], ThreadBoardList[0].CurrentColor);

            GenMoveCount = 0;
            GenerateAllMoves(&ThreadBoardList[0], NULL, NULL, MoveList, &GenMoveCount);

            continue; // Next string
        }

        if (strcmp(ReadStr, "save") == 0) {
            SaveGame(&ThreadBoardList[0]);

            continue; // Next string
        }

        if (strcmp(ReadStr, "exit") == 0) {
            return FALSE;
        }

        if (strlen(ReadStr) == 4 || strlen(ReadStr) == 5) { // Move (e2e4, e7e8q)
            File = ReadStr[0] - 'a';
            Rank = 7 - (ReadStr[1] - '1');

            From = SQUARE_CREATE(File, Rank);

            File = ReadStr[2] - 'a';
            Rank = 7 - (ReadStr[3] - '1');

            To = SQUARE_CREATE(File, Rank);

            if (ReadStr[4] == 'N' || ReadStr[4] == 'n') {
                PromotePieceType = KNIGHT;
            }
            else if (ReadStr[4] == 'B' || ReadStr[4] == 'b') {
                PromotePieceType = BISHOP;
            }
            else if (ReadStr[4] == 'R' || ReadStr[4] == 'r') {
                PromotePieceType = ROOK;
            }
            else if (ReadStr[4] == 'Q' || ReadStr[4] == 'q') {
                PromotePieceType = QUEEN;
            }
            else {
                PromotePieceType = 0;
            }

            Move = MOVE_CREATE(From, To, PromotePieceType);

            for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) {
                if (MoveList[MoveNumber].Move == Move) {
                    NotateMove(&ThreadBoardList[0], MoveList[MoveNumber], NotateMoveStr);

                    MakeMove(&ThreadBoardList[0], MoveList[MoveNumber]);

                    if (IsInCheck(&ThreadBoardList[0], CHANGE_COLOR(ThreadBoardList[0].CurrentColor))) { // Illegal move
                        UnmakeMove(&ThreadBoardList[0]);

                        break; // for
                    }

                    PrintBoard(&ThreadBoardList[0]);

                    printf("\n");

                    if (ThreadBoardList[0].CurrentColor == WHITE) {
                        printf("%d: ... %s%s", ThreadBoardList[0].HalfMoveNumber / 2, BoardName[MOVE_FROM(MoveList[MoveNumber].Move)], BoardName[MOVE_TO(MoveList[MoveNumber].Move)]);
                    }
                    else { // BLACK
                        printf("%d: %s%s", ThreadBoardList[0].HalfMoveNumber / 2 + 1, BoardName[MOVE_FROM(MoveList[MoveNumber].Move)], BoardName[MOVE_TO(MoveList[MoveNumber].Move)]);
                    }

                    if (MoveList[MoveNumber].Type & MOVE_PAWN_PROMOTE) {
                        printf("%c", PiecesCharBlack[MOVE_PROMOTE_PIECE_TYPE(MoveList[MoveNumber].Move)]);
                    }

                    printf(" (%s)\n", NotateMoveStr);

                    if (IsInsufficientMaterial(&ThreadBoardList[0])) {
                        printf("\n");

                        printf("1/2-1/2 {Draw by insufficient mating material}!\n");

                        return FALSE;
                    }

                    if (ThreadBoardList[0].FiftyMove >= 100) {
                        printf("\n");

                        printf("1/2-1/2 {Draw by fifty moves rule}!\n");

                        return FALSE;
                    }

                    if (PositionRepeat2(&ThreadBoardList[0]) == 2) {
                        printf("\n");

                        printf("1/2-1/2 {Draw by 3-fold repetition}!\n");

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
    int InputNodes;
    int InputMaxTime;
    int InputHashSize;
    int InputThreads;

    printf("\n");

    printf("Max. depth (min. 1 max. %d; 0 = %d): ", MAX_PLY, MAX_PLY);
    scanf_s("%d", &InputDepth);

    MaxDepth = (InputDepth >= 1 && InputDepth <= MAX_PLY) ? InputDepth : MAX_PLY;

    printf("Max. nodes (min. 1; 0 = No limit): ");
    scanf_s("%d", &InputNodes);

    MaxNodes = InputNodes >= 1 ? (U64)InputNodes : 0ULL;

    printf("Max. time (min. 1; 0 = No limit), sec.: ");
    scanf_s("%d", &InputMaxTime);

    MaxTime = InputMaxTime >= 1 ? (U64)InputMaxTime : 0ULL;
    MaxTime *= 1000ULL;

    if (MaxTime > 0ULL) {
        if (MaxTime > ReduceTime) {
            MaxTime -= ReduceTime;
        }
        else {
            MaxTime = 1ULL;
        }
    }

    TimeForMove = 0ULL;

    memset(TargetTime, 0, sizeof(TargetTime));

    printf("Hash table size (min. 1 max. %d; 0 = %d), Mb: ", MAX_HASH_TABLE_SIZE, DEFAULT_HASH_TABLE_SIZE);
    scanf_s("%d", &InputHashSize);

    InputHashSize = (InputHashSize >= 1 && InputHashSize <= MAX_HASH_TABLE_SIZE) ? InputHashSize : DEFAULT_HASH_TABLE_SIZE;

    InitHashTable(InputHashSize);

    printf("Threads (min. 1 max. %d; 0 = %d): ", MaxThreads, DEFAULT_THREADS);
    scanf_s("%d", &InputThreads);

    InputThreads = (InputThreads >= 1 && InputThreads <= MaxThreads) ? InputThreads : DEFAULT_THREADS;

    omp_set_num_threads(InputThreads);
}

void Game(const int HumanColor, const int ComputerColor)
{
    InputParametrs();

    PrintBoard(&ThreadBoardList[0]);

    while (TRUE) {
        if (ThreadBoardList[0].CurrentColor == HumanColor) {
            if (!HumanMove()) {
                return;
            }
        }

        if (ThreadBoardList[0].CurrentColor == ComputerColor) {
            if (!ComputerMove()) {
                return;
            }
        }
    }
}

void GameAuto(void)
{
    InputParametrs();

    PrintBoard(&ThreadBoardList[0]);

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

        return;
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

        return;
    }

    GetFen(Board, Fen);

    fprintf(File, "%s", Fen);

    fclose(File);
}