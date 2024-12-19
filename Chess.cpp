// Chess.cpp

#include "stdafx.h"

#include "BitBoard.h"
#include "Board.h"
#include "Book.h"
#include "Def.h"
#include "Game.h"
#include "Gen.h"
#include "Hash.h"
#include "NNUE2.h"
#include "Tests.h"
#include "Tuning.h"
#include "UCI.h"
#include "Utils.h"

int main(int argc, char** argv)
{
    BOOL InitHashTableResult;

    char Buf[64];

    int Choice;

    // Print program name, program version, evaluation function name and copyright information

    printf("%s %s %s\n", PROGRAM_NAME, PROGRAM_VERSION, EVALUATION_FUNCTION_NAME);
    printf("Copyright (C) %s %s\n", YEARS, AUTHOR);

    // Print debug information

//    printf("MoveItem = %zd\n", sizeof(MoveItem));
//    printf("AccumulatorItem = %zd\n", sizeof(AccumulatorItem));
//    printf("HistoryItem = %zd\n", sizeof(HistoryItem));
//    printf("BoardItem = %zd\n", sizeof(BoardItem));

//    BoardItem Board;

//    printf("BoardItem.MoveTable = %zd\n", sizeof(Board.MoveTable));
//    printf("BoardItem.BestMovesRoot = %zd\n", sizeof(Board.BestMovesRoot));
//    printf("BoardItem.HeuristicTable = %zd\n", sizeof(Board.HeuristicTable));
//    printf("BoardItem.CounterMoveHistoryTable = %zd\n", sizeof(Board.CounterMoveHistoryTable));
//    printf("BoardItem.KillerMoveTable = %zd\n", sizeof(Board.KillerMoveTable));
//    printf("BoardItem.CounterMoveTable = %zd\n", sizeof(Board.CounterMoveTable));

    // Initialize threads

    /*
        https://learn.microsoft.com/en-us/cpp/parallel/openmp/3-run-time-library-functions?view=msvc-170#microsoft-specific
    */
    omp_set_dynamic(0);

    MaxThreads = omp_get_max_threads(); // Save hardware max. threads

    MaxThreads = MIN(MaxThreads, MAX_THREADS);

    omp_set_num_threads(DEFAULT_THREADS);

    printf("\n");

    printf("Max. threads = %d\n", MaxThreads);
    printf("Threads = %d\n", DEFAULT_THREADS);

#ifdef BIND_THREAD_V1
    InitThreadNode();
#endif // BIND_THREAD_V1

    // Initialize bit boards

    InitBitBoards();

    // Initialize hash table

    InitHashTableResult = InitHashTable(DEFAULT_HASH_TABLE_SIZE);

    if (!InitHashTableResult) { // Init hash table error
        Sleep(3000);

        goto Done;
    }

    ClearHash();

    printf("\n");

    printf("Max. hash table size = %d Mb\n", MAX_HASH_TABLE_SIZE);
    printf("Hash table size = %d Mb\n", DEFAULT_HASH_TABLE_SIZE);

    // Initialize hash boards

    SetRandState(0ULL); // For reproducibility
    InitHashBoards();

    // Initialize LMP

#ifdef LATE_MOVE_PRUNING
    for (int Depth = 0; Depth < 7; ++Depth) {
        LateMovePruningTable[Depth] = (int)round(2.98484 + pow(Depth, 1.74716)); // Hakkapeliitta

//        printf("LateMovePruningTable[%d] = %d\n", Depth, LateMovePruningTable[Depth]); // 3, 4, 6, 10, 14, 20, 26
    }
#endif // LATE_MOVE_PRUNING

    // Initialize LMR

#ifdef LATE_MOVE_REDUCTION
    for (int Depth = 0; Depth < 64; ++Depth) {
//        printf("LateMoveReductionTable[%2d] = ", Depth);

        for (int MoveNumber = 0; MoveNumber < 64; ++MoveNumber) {
            LateMoveReductionTable[Depth][MoveNumber] = (int)MAX(log(Depth + 1) * log(MoveNumber + 1) / 1.70, 1.0); // Hakkapeliitta

//            printf("%d", LateMoveReductionTable[Depth][MoveNumber]);
        }

//        printf("\n");
    }
#endif // LATE_MOVE_REDUCTION

    // Initialize random generator

    SetRandState(Clock());

    // Load book

    BookFileLoaded = LoadBook(DEFAULT_BOOK_FILE_NAME);

    // Load network

    if (argc > 1) {
        NnueFileLoaded = LoadNetwork(argv[1]);
    }
    else {
        NnueFileLoaded = LoadNetwork(DEFAULT_NNUE_FILE_NAME);
    }

    // UCI or Terminal User Interface (TUI)?

    printf("\n");

    printf("Use UCI commands or press Enter to display the menu\n");

    fgets(Buf, sizeof(Buf), stdin);

    if (strncmp(Buf, "uci", 3) == 0) { // UCI
        PrintMode = PRINT_MODE_UCI;

        UCI();

        PrintMode = PRINT_MODE_NORMAL;

        goto Done;
    }

    // Terminal User Interface (TUI)

    if (!NnueFileLoaded) {
        printf("Network not loaded!\n");

        Sleep(3000);

        goto Done;
    }

    while (TRUE) {
        printf("Menu:\n");

        printf("\n");

        printf(" 1: New white game\n");
        printf(" 2: New black game\n");
        printf(" 3: New auto game\n");

        printf(" 4: Load game from file (chess.fen) and white game\n");
        printf(" 5: Load game from file (chess.fen) and black game\n");
        printf(" 6: Load game from file (chess.fen) and auto game\n");

        printf(" 7: Built-in drive generator test\n");
        printf(" 8: Load game from file (chess.fen) and drive generator test\n");

        printf(" 9: Bratko-Kopec test (24 positions)\n");
        printf("10: Win-At-Chess test (300 positions)\n");

        printf("11: Built-in performance test\n");
        printf("12: Built-in performance test (evaluate function)\n");

        printf("13: Generate book file (book.txt) from PGN file (book.pgn)\n");

        printf("14: Convert PGN file (games.pgn) to FEN file (games.fen)\n");

        printf("15: Exit\n");

        printf("\n");

        printf("Choice: ");
        scanf_s("%d", &Choice);

        switch (Choice) {
            case 1:
                SetFen(&CurrentBoard, StartFen);
                Game(WHITE, BLACK);
                break;

            case 2:
                SetFen(&CurrentBoard, StartFen);
                Game(BLACK, WHITE);
                break;

            case 3:
                SetFen(&CurrentBoard, StartFen);
                GameAuto();
                break;

            case 4:
                LoadGame(&CurrentBoard);
                Game(WHITE, BLACK);
                break;

            case 5:
                LoadGame(&CurrentBoard);
                Game(BLACK, WHITE);
                break;

            case 6:
                LoadGame(&CurrentBoard);
                GameAuto();
                break;

            case 7:
                GeneratorTest1();
                break;

            case 8:
                LoadGame(&CurrentBoard);
                GeneratorTest2();
                break;

            case 9:
                BratkoKopecTest();
                break;

            case 10:
                WinAtChessTest();
                break;

            case 11:
                PerformanceTest();
                break;

            case 12:
                PerformanceTestEvaluate();
                break;

            case 13:
                GenerateBook();
                break;

            case 14:
                Pgn2Fen();
                break;

            case 15: // Exit
                goto Done;
        } // switch

        printf("\n");
    } // while

Done:

    if (BookFileLoaded) {
        free(BookStore.Item);
    }

    free(HashStore.Item);

    return 0;
}