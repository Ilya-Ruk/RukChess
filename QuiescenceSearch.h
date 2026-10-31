// QuiescenceSearch.h

#pragma once

#ifndef QUIESCENCE_SEARCH_H
#define QUIESCENCE_SEARCH_H

#include "Board.h"
#include "Def.h"
#include "Types.h"

int QuiescenceSearch(BoardItem* Board, SearchDataItem* SearchData, int Alpha, int Beta, const int Depth, const int Ply, const BOOL IsPrincipal, const BOOL InCheck);

#endif // !QUIESCENCE_SEARCH_H