// Utils.h

#pragma once

#ifndef UTILS_H
#define UTILS_H

#include "Def.h"
#include "Types.h"

#define ABS(Value)          ((Value) < 0 ? -(Value) : (Value))

#define MAX(First, Second)  ((First) >= (Second) ? (First) : (Second))
#define MIN(First, Second)  ((First) < (Second) ? (First) : (Second))

#define SQR(Value)          ((Value) * (Value))

/*
    Time in milliseconds since midnight (00:00:00), January 1, 1970, coordinated universal time (UTC)
*/
U64 Clock(void);

/*
    https://prng.di.unimi.it/splitmix64.c
*/
U64 Rand64(void);

void SetRandState(const U64 NewRandState);

#ifdef BIND_THREAD_V1
void InitThreadNode(void);

void BindThread(const int ThreadNumber);
#endif // BIND_THREAD_V1

#ifdef BIND_THREAD_V2
void BindThread(const int ThreadNumber);
#endif // BIND_THREAD_V2

#endif // !UTILS_H