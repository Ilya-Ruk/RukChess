// Game.h

#pragma once

#ifndef GAME_H
#define GAME_H

#include "Board.h"
#include "Def.h"
#include "Types.h"

#define PRINT_MODE_NORMAL   0
#define PRINT_MODE_UCI      1
#define PRINT_MODE_TESTS    2

extern int MaxThreads;
extern int MaxDepth;
extern U64 MaxTime;

extern U64 ReduceTime;

extern U64 TimeForMove;

extern U64 TimeStart;
extern U64 TimeStop;
extern U64 TotalTime;

extern int TimeStep;
extern U64 TargetTime[MAX_TIME_STEPS];

extern int CompletedDepth;

extern volatile BOOL StopSearch;

extern int PrintMode;

void PrintBestMoves(const BoardItem* Board, const int Depth, const MoveItem* BestMoves, const int BestScore);
void SaveBestMoves(MoveItem* BestMoves, const MoveItem BestMove, const MoveItem* TempBestMoves);

BOOL ComputerMove(void);
void ComputerMoveThread(void* ignored);

BOOL HumanMove(void);

void InputParametrs(void);

void Game(const int HumanColor, const int ComputerColor);
void GameAuto(void);

void LoadGame(BoardItem* Board);
void SaveGame(const BoardItem* Board);

#endif // !GAME_H