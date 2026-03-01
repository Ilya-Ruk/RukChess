// Hash.h

#pragma once

#ifndef HASH_H
#define HASH_H

#include "Board.h"
#include "Def.h"
#include "Types.h"

#define HASH_EXACT          1
#define HASH_ALPHA          2
#define HASH_BETA           4
#define HASH_STATIC_SCORE   8

typedef struct {
    I16 Score;
    I16 StaticScore;

    U16 Move;

    I8 Depth;

    U8 Flag : 4, Iteration : 4;
} HashDataS; // 8 bytes

typedef union {
    U64 RawData;

    HashDataS Data;
} HashDataU; // 8 bytes

typedef struct {
    U64 KeyValue;

    HashDataU Value;
} HashItem; // 16 bytes

typedef struct {
    U64 Size;
    U64 Mask;

    U8 Iteration; // 4 bits

    HashItem* Item;
} HashStoreItem; // 32 bytes

extern HashStoreItem HashStore;

extern U64 PieceHash[2][6][64]; // [Color][Piece][Square]
extern U64 ColorHash;
extern U64 PassantHash[64];     // [Square]

BOOL InitHashTable(const int SizeInMb); // Xiphos
void ClearHashTable(void);
void FreeHashTable(void);

void InitHashBoards(void);

void InitHash(BoardItem* Board);

void AddHashStoreIteration(void);

void SaveHash(const U64 Hash, const int Depth, const int Ply, const int Score, const int StaticScore, const int Move, const int Flag);
void LoadHash(const U64 Hash, int* Depth, const int Ply, int* Score, int* StaticScore, int* Move, int* Flag);

int FullHash(void);

#ifdef HASH_PREFETCH
void Prefetch(const U64 Hash);
#endif // HASH_PREFETCH

#endif // !HASH_H