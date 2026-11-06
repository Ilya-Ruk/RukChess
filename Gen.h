// Gen.h

#pragma once

#ifndef GEN_H
#define GEN_H

#include "Board.h"
#include "Def.h"

void GenerateAllMoves(const BoardItem* Board, const SearchDataItem* SearchData, const int** CMH_Pointer, MoveItem* MoveList, int* GenMoveCount);
void GenerateCaptureMoves(const BoardItem* Board, const SearchDataItem* SearchData, const int** CMH_Pointer, MoveItem* MoveList, int* GenMoveCount);

void GenerateAllLegalMoves(BoardItem* Board, const SearchDataItem* SearchData, const int** CMH_Pointer, MoveItem* LegalMoveList, int* LegalMoveCount);

#endif // !GEN_H