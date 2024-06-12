// stdafx.h

#pragma once

#ifndef STDAFX_H
#define STDAFX_H

#pragma warning(disable: 4100)

#include "targetver.h"

#include <stdio.h>      // _IONBF, printf(), scanf_s(), fopen_s(), fgets(), fprintf(), fclose(), fseek(), sprintf_s(), fread_s()
#include <stdlib.h>     // atoi(), atof(), malloc(), realloc(), calloc(), free(), qsort()
#include <string.h>     // strcmp(), strncmp(), strstr(), strcpy_s(), strchr()
#include <sys/timeb.h>  // _timeb, _ftime_s()
#include <windows.h>    // GetLogicalProcessorInformationEx(), GetNumaNodeProcessorMaskEx(), GetCurrentThread(), Sleep()
#include <process.h>    // _beginthread(), _endthread()
#include <omp.h>        // Open MP
#include <intrin.h>     // __popcnt64(), _BitScanForward64(), _BitScanReverse64(), _mm_prefetch()
#include <immintrin.h>  // _pdep_u64(), _pext_u64()
#include <float.h>      // DBL_MAX
#include <limits.h>     // INT_MAX
#include <math.h>       // round(), pow(), log(), sqrt()

#endif // !STDAFX_H