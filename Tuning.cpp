// Tuning.cpp

#include "stdafx.h"

#include "Tuning.h"

#include "Board.h"
#include "Def.h"
#include "Game.h"
#include "Gen.h"
#include "Hash.h"
#include "Move.h"
#include "Types.h"

#define STAGE_NONE              1
#define STAGE_TAG               2
#define STAGE_NOTATION          3
#define STAGE_MOVE              4
#define STAGE_COMMENT           5

#define MIN_USE_PLY             0       // 0 moves

void Pgn2Fen(void)
{
    FILE* FileIn;
    FILE* FileOut;

    int GameNumber;

    double Result;

    int Ply;

    BOOL Error;

    char MoveString[16];
    char* Move;

    int Stage;

    char Buf[4096];
    char* Part;

    char FenString[MAX_FEN_LENGTH];
    char* Fen;

    char FenOut[MAX_FEN_LENGTH];

    int GenMoveCount;
    MoveItem MoveList[MAX_GEN_MOVES];

    BOOL MoveFound;

    char NotateMoveStr[16];

    printf("\n");

    printf("Read PGN file...\n");

    fopen_s(&FileIn, "games.pgn", "r");

    if (FileIn == NULL) { // File open error
        printf("File 'games.pgn' open error!\n");

        Sleep(3000);

        exit(0);
    }

    fopen_s(&FileOut, "games.fen", "w");

    if (FileOut == NULL) { // File create (open) error
        printf("File 'games.fen' create (open) error!\n");

        Sleep(3000);

        exit(0);
    }

    // Cache not used

    InitHashTable(1);
    ClearHash();

    // Threads not used

    omp_set_num_threads(1);

    // Prepare new game

    GameNumber = 1;

    Result = 0.0; // Draw default

    Ply = 0;

    Error = FALSE;

    Move = MoveString;
    *Move = '\0'; // Nul

    SetFen(&CurrentBoard, StartFen);

    Stage = STAGE_NONE;

    printf("\n");

    while (fgets(Buf, sizeof(Buf), FileIn) != NULL) {
        Part = Buf;

        if (*Part == '\r' || *Part == '\n') { // Empty string
            if (Stage == STAGE_NOTATION) {
                // Prepare new game

                ++GameNumber;

                Result = 0.0; // Draw default

                Ply = 0;

                Error = FALSE;

                Move = MoveString;
                *Move = '\0'; // Nul

                SetFen(&CurrentBoard, StartFen);

                Stage = STAGE_NONE;
            }

            continue; // Next string
        }

        if (*Part == '[') { // Tag
            if (Stage == STAGE_NONE) {
                if (GameNumber > 1 && ((GameNumber - 1) % 1000) == 0) {
                    printf("Game number = %d\n", GameNumber - 1);
                }

                Stage = STAGE_TAG;
            }

            if (!strncmp(Part, "[Result \"1-0\"]", 14)) { // Result 1-0
                Result = 1.0;

//                printf("Result = %.1f\n", Result);
            }
            else if (!strncmp(Part, "[Result \"1/2-1/2\"]", 18)) { // Result 1/2-1/2
                Result = 0.5;

//                printf("Result = %.1f\n", Result);
            }
            else if (!strncmp(Part, "[Result \"0-1\"]", 14)) { // Result 0-1
                Result = 0.0;

//                printf("Result = %.1f\n", Result);
            }
            else if (!strncmp(Part, "[FEN \"", 6)) { // FEN
                Part += 6;

                Fen = FenString;

                while (*Part != '"') {
                    *Fen++ = *Part++; // Copy FEN
                }

                *Fen = '\0'; // Nul

//                printf("FEN = %s\n", FenString);

                SetFen(&CurrentBoard, FenString);
            }

            continue; // Next string
        } // if

        if (Stage == STAGE_TAG) {
            Stage = STAGE_NOTATION;

            if (Ply >= MIN_USE_PLY) {
                GetFen(&CurrentBoard, FenOut); // First FEN in game (StartFen or FenString from FEN-tag)

                fprintf(FileOut, "%s|%.1f\n", FenOut, Result);
            }

            ++Ply;
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
                    Stage = STAGE_NOTATION;

                    *Move = '\0'; // Nul

                    if (Error) {
                        ++Part;

                        continue; // Next character in string
                    }

//                    printf("Move = %s\n", MoveString);

                    GenMoveCount = 0;
                    GenerateAllMoves(&CurrentBoard, NULL, MoveList, &GenMoveCount);

                    MoveFound = FALSE;

                    for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) {
                        NotateMove(&CurrentBoard, MoveList[MoveNumber], NotateMoveStr);

                        if (strcmp(MoveString, NotateMoveStr) == 0) {
                            MakeMove(&CurrentBoard, MoveList[MoveNumber]);

                            if (IsInCheck(&CurrentBoard, CHANGE_COLOR(CurrentBoard.CurrentColor))) { // Illegal move
                                UnmakeMove(&CurrentBoard);

                                PrintBoard(&CurrentBoard);

                                printf("\n");

                                printf("Move = %s check\n", MoveString);

                                printf("\n");

                                printf("Gen. move = %s%s", BoardName[MOVE_FROM(MoveList[MoveNumber].Move)], BoardName[MOVE_TO(MoveList[MoveNumber].Move)]);

                                if (MoveList[MoveNumber].Type & MOVE_PAWN_PROMOTE) {
                                    printf("%c", PiecesCharBlack[MOVE_PROMOTE_PIECE(MoveList[MoveNumber].Move)]);
                                }

                                printf(" (%s)\n\n", NotateMoveStr);

                                continue; // Next move
                            }

                            MoveFound = TRUE;

                            break; // for
                        }
                    }

                    if (MoveFound) {
                        if (Ply >= MIN_USE_PLY) {
                            GetFen(&CurrentBoard, FenOut);

                            fprintf(FileOut, "%s|%.1f\n", FenOut, Result);
                        }
                    }
                    else { // Move not found
                        Error = TRUE;

                        PrintBoard(&CurrentBoard);

                        printf("\n");

                        printf("Move = %s not found\n", MoveString);

                        printf("\n");

                        printf("Gen. moves =");

                        for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) {
                            NotateMove(&CurrentBoard, MoveList[MoveNumber], NotateMoveStr);

                            printf(" %s%s", BoardName[MOVE_FROM(MoveList[MoveNumber].Move)], BoardName[MOVE_TO(MoveList[MoveNumber].Move)]);

                            if (MoveList[MoveNumber].Type & MOVE_PAWN_PROMOTE) {
                                printf("%c", PiecesCharBlack[MOVE_PROMOTE_PIECE(MoveList[MoveNumber].Move)]);
                            }

                            printf(" (%s)", NotateMoveStr);
                        }

                        printf("\n\n");
                    }

                    ++Ply;
                }
            }

            ++Part;
        } // while
    } // while

    printf("Game number = %d\n", GameNumber - 1);

    fclose(FileOut);
    fclose(FileIn);

    printf("\n");

    printf("Read PGN file...DONE\n");
}