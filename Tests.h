// Tests.h

#pragma once

#ifndef TESTS_H
#define TESTS_H

#include "Def.h"
#include "Types.h"

typedef struct {
    char* Fen;
    int Depth;
    U64 Nodes[10];
} GeneratorTestItem;

void GeneratorTest1(void);
void GeneratorTest2(void);

void BratkoKopecTest(void);
void WinAtChessTest(void);

void SearchPerformanceTest(void);
void EvaluatePerformanceTest(void);

#endif // !TESTS_H