// Evaluate.h

#pragma once

#ifndef EVALUATE_H
#define EVALUATE_H

#include "Board.h"
#include "Def.h"
#include "Types.h"

void InitEvaluation(void);

int Evaluate(BoardItem* Board);

#endif // !EVALUATE_H