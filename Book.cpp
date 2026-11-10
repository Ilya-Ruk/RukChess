// Book.cpp

#include "stdafx.h"

#include "Book.h"

#include "Board.h"
#include "Def.h"
#include "Game.h"
#include "Gen.h"
#include "Hash.h"
#include "Move.h"
#include "Types.h"
#include "Utils.h"

#define STAGE_NONE      1
#define STAGE_TAG       2
#define STAGE_NOTATION  3
#define STAGE_MOVE      4
#define STAGE_COMMENT   5

#define MAX_BOOK_PLY    24 // 12 moves
#define MIN_BOOK_GAMES  30

BookStoreItem BookStore;

BOOL BookFileLoaded = FALSE;

NodeItem* CreateNode(const MoveItem Move)
{
    NodeItem* Node = (NodeItem*)calloc(1, sizeof(NodeItem));

    if (Node == NULL) { // Allocate memory error
        printf("Allocate memory to node error!\n");

        return NULL;
    }

    Node->Move = Move;

    return Node;
}

void FreeNode(NodeItem* Node)
{
    for (int Index = 0; Index < MAX_CHILDREN; Index++) {
        NodeItem* ChildNode = Node->Children[Index];

        if (ChildNode) {
            FreeNode(ChildNode);
        }
    }

    free(Node);
}

void PrintNode(BoardItem* Board, const NodeItem* Node, FILE* FileOut)
{
    int Total = 0;

    for (int Index = 0; Index < MAX_CHILDREN; Index++) {
        NodeItem* ChildNode = Node->Children[Index];

        if (ChildNode) {
            Total += ChildNode->Total;
        }
    }

    if (Total < MIN_BOOK_GAMES) {
        return;
    }

    for (int Index = 0; Index < MAX_CHILDREN; Index++) {
        NodeItem* ChildNode = Node->Children[Index];

        if (ChildNode) {
            fprintf(FileOut, "0x%016llx %d %d %d\n", Board->Hash, MOVE_FROM(ChildNode->Move.Move), MOVE_TO(ChildNode->Move.Move), ChildNode->Total);

            MakeMove(Board, ChildNode->Move);

            PrintNode(Board, ChildNode, FileOut);

            UnmakeMove(Board);
        }
    }
}

void GenerateBook(void)
{
    int GameNumber;

    int Result;

    char MoveString[16];
    char* Move;

    int Ply;

    NodeItem* Node;

    BOOL Error;

    int Stage;

    printf("\n");

    printf("Generate book file from PGN file...\n");

    NodeItem* RootNode = CreateNode((MoveItem){ 0, 0, 0 });

    if (RootNode == NULL) { // Create node error
        return;
    }

    FILE* FileIn;

    fopen_s(&FileIn, "book.pgn", "r");

    if (FileIn == NULL) { // File open error
        printf("File 'book.pgn' open error!\n");

        FreeNode(RootNode);

        return;
    }

    FILE* FileOut;

    fopen_s(&FileOut, DEFAULT_BOOK_FILE_NAME, "w");

    if (FileOut == NULL) { // File create (open) error
        printf("File '%s' create (open) error!\n", DEFAULT_BOOK_FILE_NAME);

        fclose(FileIn);

        FreeNode(RootNode);

        return;
    }

    // Cache not used

    InitHashTable(1);

    // Threads not used

    omp_set_num_threads(1);

    // Prepare new game

    GameNumber = 1;

    Result = 0; // Draw default

    Move = MoveString;
    *Move = '\0'; // Nul

    Ply = 0;

    Node = RootNode;

    Error = FALSE;

    SetFen(&BoardList[0], StartFen);

    Stage = STAGE_NONE;

    char Buf[4096];

    while (fgets(Buf, sizeof(Buf), FileIn) != NULL) {
        const char* Part = Buf;

        if (*Part == '\r' || *Part == '\n') { // Empty string
            if (Stage == STAGE_NOTATION) {
                // Prepare new game

                GameNumber++;

                Result = 0; // Draw default

                Move = MoveString;
                *Move = '\0'; // Nul

                Ply = 0;

                Node = RootNode;

                Error = FALSE;

                SetFen(&BoardList[0], StartFen);

                Stage = STAGE_NONE;
            }

            continue; // Next string
        }

        if (*Part == '[') { // Tag
            if (Stage == STAGE_NONE) {
                if (GameNumber > 1 && ((GameNumber - 1) % 10000) == 0) {
                    printf("\n");

                    printf("Game number = %d\n", GameNumber - 1);

                    printf("\n");

                    printf("  White = %d Black = %d Draw = %d Total = %d\n", RootNode->White, RootNode->Black, RootNode->Draw, RootNode->Total);
                    printf("  White = %.1f%% Black = %.1f%% Draw = %.1f%%\n", 100.0 * (double)RootNode->White / (double)RootNode->Total, 100.0 * (double)RootNode->Black / (double)RootNode->Total, 100.0 * (double)RootNode->Draw / (double)RootNode->Total);
                    printf("  White score = %.1f%%\n", 100.0 * ((double)RootNode->White + (double)RootNode->Draw / 2.0) / (double)RootNode->Total);
                }

                Stage = STAGE_TAG;
            }

            if (strncmp(Part, "[Result \"1-0\"]", 14) == 0) { // Result 1-0
                Result = 1; // White win
            }
            else if (strncmp(Part, "[Result \"1/2-1/2\"]", 18) == 0) { // Result 1/2-1/2
                Result = 0; // Draw
            }
            else if (strncmp(Part, "[Result \"0-1\"]", 14) == 0) { // Result 0-1
                Result = -1; // Black win
            }
            else if (strncmp(Part, "[FEN \"", 6) == 0) { // FEN
                Part += 6;

                char FenString[MAX_FEN_LENGTH];
                char* Fen = FenString;

                while (*Part != '"') {
                    *Fen++ = *Part++; // Copy FEN
                }

                *Fen = '\0'; // Nul

                SetFen(&BoardList[0], FenString);
            }

            continue; // Next string
        } // if

        if (Stage == STAGE_TAG) {
            if (Result == 1) { // White win
                RootNode->White++;

                RootNode->Total++;
            }
            else if (Result == 0) { // Draw
                RootNode->Draw++;

                RootNode->Total++;
            }
            else if (Result == -1) { // Black win
                RootNode->Black++;

                RootNode->Total++;
            }

            Stage = STAGE_NOTATION;
        }

        if (Error || Ply >= MAX_BOOK_PLY) {
            continue; // Next string
        }

        while (*Part != '\0') { // Scan string
            if (*Part == '{') { // Comment (open)
                Stage = STAGE_COMMENT;
            }
            else if (*Part == '}') { // Comment (close)
                Stage = STAGE_NOTATION;
            }

            if (Stage == STAGE_NOTATION) {
                if (strchr(MoveFirstChar, *Part) != NULL) {
                    Stage = STAGE_MOVE;

                    Move = MoveString;

                    *Move++ = *Part; // Copy move (first char)
                }
            }
            else if (Stage == STAGE_MOVE) {
                if (strchr(MoveSubsequentChar, *Part) != NULL) {
                    *Move++ = *Part; // Copy move (subsequent char)
                }
                else { // End of move
                    *Move = '\0'; // Nul

                    Stage = STAGE_NOTATION;

                    if (Error || Ply >= MAX_BOOK_PLY) {
                        Part++;

                        continue; // Next character in string
                    }

                    int GenMoveCount;
                    MoveItem MoveList[MAX_GEN_MOVES];

                    GenMoveCount = 0;
                    GenerateAllMoves(&BoardList[0], NULL, NULL, MoveList, &GenMoveCount);

                    BOOL MoveFound = FALSE;

                    for (int MoveNumber = 0; MoveNumber < GenMoveCount; MoveNumber++) {
                        char NotateMoveStr[16];

                        NotateMove(&BoardList[0], MoveList[MoveNumber], NotateMoveStr);

                        if (strcmp(MoveString, NotateMoveStr) == 0) {
                            MakeMove(&BoardList[0], MoveList[MoveNumber]);

                            if (IsInCheck(&BoardList[0], CHANGE_COLOR(BoardList[0].CurrentColor))) { // Illegal move
                                UnmakeMove(&BoardList[0]);

                                PrintBoard(&BoardList[0]);

                                printf("\n");

                                printf("Illegal move (%s)!\n", MoveString);

                                continue; // Next move
                            }

                            MoveFound = TRUE;

                            NodeItem* ChildNode = NULL;

                            for (int Index = 0; Index < MAX_CHILDREN; Index++) {
                                if (Node->Children[Index]) {
                                    if (Node->Children[Index]->Move.Move == MoveList[MoveNumber].Move) {
                                        ChildNode = Node->Children[Index];

                                        break; // for (children)
                                    }
                                }
                                else {
                                    ChildNode = CreateNode(MoveList[MoveNumber]);

                                    if (ChildNode == NULL) { // Create node error
                                        break; // for (children)
                                    }

                                    Node->Children[Index] = ChildNode;

                                    break; // for (children)
                                }
                            }

                            if (ChildNode == NULL) {
                                Error = TRUE;

                                printf("\n");

                                printf("No child node!\n");
                            }
                            else {
                                if (Result == 1) { // White win
                                    ChildNode->White++;

                                    ChildNode->Total++;
                                }
                                else if (Result == 0) { // Draw
                                    ChildNode->Draw++;

                                    ChildNode->Total++;
                                }
                                else if (Result == -1) { // Black win
                                    ChildNode->Black++;

                                    ChildNode->Total++;
                                }

                                Node = ChildNode;
                            }

                            break; // for (moves)
                        } // if
                    } // for

                    if (!MoveFound) { // Move not found
                        Error = TRUE;

                        PrintBoard(&BoardList[0]);

                        printf("\n");

                        printf("Move (%s) not found!\n", MoveString);

                        printf("\n");

                        printf("Gen. moves:");

                        for (int MoveNumber = 0; MoveNumber < GenMoveCount; MoveNumber++) {
                            char NotateMoveStr[16];

                            NotateMove(&BoardList[0], MoveList[MoveNumber], NotateMoveStr);

                            printf(" %s%s", BoardName[MOVE_FROM(MoveList[MoveNumber].Move)], BoardName[MOVE_TO(MoveList[MoveNumber].Move)]);

                            if (MoveList[MoveNumber].Type & MOVE_PAWN_PROMOTE) {
                                printf("%c", PiecesCharBlack[MOVE_PROMOTE_PIECE_TYPE(MoveList[MoveNumber].Move)]);
                            }

                            printf(" (%s)", NotateMoveStr);
                        }

                        printf("\n");
                    }

                    Ply++;
                }
            }

            Part++;
        } // while
    } // while

    printf("\n");

    printf("Game number = %d\n", GameNumber - 1);

    printf("\n");

    printf("  White = %d Black = %d Draw = %d Total = %d\n", RootNode->White, RootNode->Black, RootNode->Draw, RootNode->Total);
    printf("  White = %.1f%% Black = %.1f%% Draw = %.1f%%\n", 100.0 * (double)RootNode->White / (double)RootNode->Total, 100.0 * (double)RootNode->Black / (double)RootNode->Total, 100.0 * (double)RootNode->Draw / (double)RootNode->Total);
    printf("  White score = %.1f%%\n", 100.0 * ((double)RootNode->White + (double)RootNode->Draw / 2.0) / (double)RootNode->Total);

    // Prepare new game

    SetFen(&BoardList[0], StartFen);

    PrintNode(&BoardList[0], RootNode, FileOut);

    fclose(FileOut);
    fclose(FileIn);

    FreeNode(RootNode);

    printf("\n");

    printf("Generate book file from PGN file...DONE\n");
}

int HashCompare(const void* BookItem1, const void* BookItem2)
{
    const U64 Hash1 = ((BookItem*)BookItem1)->Hash;
    const U64 Hash2 = ((BookItem*)BookItem2)->Hash;

    if (Hash1 < Hash2) {
        return -1;
    }

    if (Hash1 > Hash2) {
        return 1;
    }

    return 0;
}

void LoadBook(const char* BookFileName)
{
    char Buf[256];

    printf("\n");

    printf("Load book...\n");

    FreeBook(); // The book may have been loaded earlier

    FILE* File;

    fopen_s(&File, BookFileName, "r");

    if (File == NULL) { // File open error
        printf("File '%s' open error!\n", BookFileName);

        return;
    }

    // The first cycle to get the number of book items

    BookStore.Count = 0;

    while (fgets(Buf, sizeof(Buf), File) != NULL) {
        BookStore.Count++;
    }

    // Allocate memory to store book

    BookStore.Item = (BookItem*)calloc(BookStore.Count, sizeof(BookItem));

    if (BookStore.Item == NULL) { // Allocate memory error
        printf("Allocate memory to store book error!\n");

        fclose(File);

        return;
    }

    // Set the pointer to the beginning of the file

    fseek(File, 0, SEEK_SET);

    // The second cycle to read book items

    int PositionNumber = 0;

    while (fgets(Buf, sizeof(Buf), File) != NULL) {
        const char* Part = Buf;

        char HashString[64];
        char* HashPointer = HashString;

        while (*Part != ' ') {
            *HashPointer++ = *Part++; // Copy hash
        }

        *HashPointer = '\0'; // Nul

        U64 Hash = strtoull(HashString, NULL, 16);

        if (Hash == 0ULL) { // Hash error
            printf("Read book error!\n");

            free(BookStore.Item);

            fclose(File);

            return;
        }

        Part++; // Space

        int From = atoi(Part);

        if (From < 0 || From > 63) {
            printf("Read book error!\n");

            free(BookStore.Item);

            fclose(File);

            return;
        }

        while (*Part != ' ') {
            Part++;
        }

        Part++; // Space

        int To = atoi(Part);

        if (To < 0 || To > 63) {
            printf("Read book error!\n");

            free(BookStore.Item);

            fclose(File);

            return;
        }

        while (*Part != ' ') {
            Part++;
        }

        Part++; // Space

        int Total = atoi(Part);

        BookItem* BookItemPointer = &BookStore.Item[PositionNumber++];

        BookItemPointer->Hash = Hash;

        BookItemPointer->From = From;
        BookItemPointer->To = To;

        BookItemPointer->Total = Total;
    } // while

    qsort(BookStore.Item, BookStore.Count, sizeof(BookItem), HashCompare);

    fclose(File);

    BookFileLoaded = TRUE;

    printf("Load book...DONE (%d)\n", BookStore.Count);
}

void FreeBook(void)
{
    if (BookFileLoaded) {
        free(BookStore.Item);

        BookFileLoaded = FALSE;
    }
}

BOOL GetBookMove(const BoardItem* Board, MoveItem* BestMove)
{
    if (!BookFileLoaded) {
        return FALSE;
    }

    int FirstIndex = -1;
    int BookCount = 0;

    int Total = 0;

    for (int Index = 0; Index < BookStore.Count; Index++) {
        const BookItem* BookItemPointer = &BookStore.Item[Index];

        if (BookItemPointer->Hash == Board->Hash) { // Position hash found in book
            if (FirstIndex < 0) {
                FirstIndex = Index;
            }

            BookCount++;

            Total += BookItemPointer->Total;
        }
    }

    if (BookCount == 0) { // No moves in book
        return FALSE;
    }

    // Weighted random choice

    U64 RandomValue = Rand64();

    int Selected = ((int)RandomValue & 0x7FFFFFFF) % Total;

    int Offset = 0;

    for (int Index = FirstIndex, Count = 0; Count < BookCount; Index++, Count++) {
        const BookItem* BookItemPointer = &BookStore.Item[Index];

        Offset += BookItemPointer->Total;

        if (Selected < Offset) {
            int GenMoveCount;
            MoveItem MoveList[MAX_GEN_MOVES];

            GenMoveCount = 0;
            GenerateAllMoves(Board, NULL, NULL, MoveList, &GenMoveCount);

            for (int MoveNumber = 0; MoveNumber < GenMoveCount; MoveNumber++) {
                if (
                    MOVE_FROM(MoveList[MoveNumber].Move) == BookItemPointer->From
                    && MOVE_TO(MoveList[MoveNumber].Move) == BookItemPointer->To
                ) { // Valid book move
                    *BestMove = MoveList[MoveNumber];

                    return TRUE;
                }
            }

            return FALSE;
        }
    }

    return FALSE;
}