// Tuning.h

#pragma once

#ifndef TUNING_H
#define TUNING_H

#include "Def.h"

void Pgn2Fen(void);

void FindBestK(void);

void InitTuningParams(void);
void LoadTuningParams(void);
void SaveTuningParams(void);

void TuningLocalSearch(void);

#endif // !TUNING_H