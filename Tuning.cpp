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

#define STAGE_NONE      1
#define STAGE_TAG       2
#define STAGE_NOTATION  3
#define STAGE_MOVE      4
#define STAGE_COMMENT   5

void Pgn2Fen(void)
{
    int GameNumber;

    double Result;

    char MoveString[16];
    char* Move;

    int Ply;

    BOOL Error;

    int Stage;

    char FenOut[MAX_FEN_LENGTH];

    printf("\n");

    printf("Convert PGN file to FEN file...\n");

    FILE* FileIn;

    fopen_s(&FileIn, "games.pgn", "r");

    if (FileIn == NULL) { // File open error
        printf("File 'games.pgn' open error!\n");

        return;
    }

    FILE* FileOut;

    fopen_s(&FileOut, "games.fen", "w");

    if (FileOut == NULL) { // File create (open) error
        printf("File 'games.fen' create (open) error!\n");

        fclose(FileIn);

        return;
    }

    // Cache not used

    InitHashTable(1);

    // Threads not used

    omp_set_num_threads(1);

    // Prepare new game

    GameNumber = 1;

    Result = 0.0; // Draw default

    Move = MoveString;
    *Move = '\0'; // Nul

    Ply = 0;

    Error = FALSE;

    SetFen(&BoardList[0], StartFen);

    Stage = STAGE_NONE;

    printf("\n");

    char Buf[4096];

    while (fgets(Buf, sizeof(Buf), FileIn) != NULL) {
        const char* Part = Buf;

        if (*Part == '\r' || *Part == '\n') { // Empty string
            if (Stage == STAGE_NOTATION) {
                // Prepare new game

                GameNumber++;

                Result = 0.0; // Draw default

                Move = MoveString;
                *Move = '\0'; // Nul

                Ply = 0;

                Error = FALSE;

                SetFen(&BoardList[0], StartFen);

                Stage = STAGE_NONE;
            }

            continue; // Next string
        }

        if (*Part == '[') { // Tag
            if (Stage == STAGE_NONE) {
                if (GameNumber > 1 && ((GameNumber - 1) % 10000) == 0) {
                    printf("Game number = %d\n", GameNumber - 1);
                }

                Stage = STAGE_TAG;
            }

            if (strncmp(Part, "[Result \"1-0\"]", 14) == 0) { // Result 1-0
                Result = 1.0; // White win
            }
            else if (strncmp(Part, "[Result \"1/2-1/2\"]", 18) == 0) { // Result 1/2-1/2
                Result = 0.5; // Draw
            }
            else if (strncmp(Part, "[Result \"0-1\"]", 14) == 0) { // Result 0-1
                Result = 0.0; // Black win
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
            GetFen(&BoardList[0], FenOut); // First FEN in game (StartFen or FenString from FEN-tag)

            fprintf(FileOut, "%s|%.1f\n", FenOut, Result);

            Ply++;

            Stage = STAGE_NOTATION;
        }

        if (Error) {
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

                    if (Error) {
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

                            break; // for
                        }
                    }

                    if (MoveFound) {
                        GetFen(&BoardList[0], FenOut);

                        fprintf(FileOut, "%s|%.1f\n", FenOut, Result);
                    }
                    else { // Move not found
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

    printf("Game number = %d\n", GameNumber - 1);

    fclose(FileOut);
    fclose(FileIn);

    printf("\n");

    printf("Convert PGN file to FEN file...DONE\n");
}