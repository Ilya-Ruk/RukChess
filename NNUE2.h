// NNUE2.h

#pragma once

#ifndef NNUE2_H
#define NNUE2_H

#include "Board.h"
#include "Def.h"
#include "Types.h"

void LoadNetwork(const char* NnueFileName);

BOOL IsNetworkLoaded(void);

int Evaluate(BoardItem* Board);

#endif // !NNUE2_H