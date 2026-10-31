// Search.h

#pragma once

#ifndef SEARCH_H
#define SEARCH_H

#include "Board.h"
#include "Def.h"
#include "Types.h"

int Search(BoardItem* Board, SearchDataItem* SearchData, int Alpha, int Beta, int Depth, const int Ply, MoveItem* BestMoves, const BOOL IsPrincipal, const BOOL InCheck, const BOOL UsePruning, const int SkipMove);

#endif // !SEARCH_H