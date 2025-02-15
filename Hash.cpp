// Hash.cpp

#include "stdafx.h"

#include "Hash.h"

#include "BitBoard.h"
#include "Def.h"
#include "Types.h"
#include "Utils.h"

HashStoreItem HashStore;

U64 PieceHash[2][6][64];    // [Color][Piece][Square]
U64 ColorHash;
U64 PassantHash[64];        // [Square]

BOOL InitHashTable(const int SizeInMb) // Xiphos
{
    U64 Items;
    U64 RoundItems = 1ULL;

    HashItem* HashItemPointer;

    Items = ((U64)SizeInMb << 20) / sizeof(HashItem);

    while (Items >>= 1) {
        RoundItems <<= 1;
    }

    HashStore.Size = RoundItems * sizeof(HashItem);
    HashStore.Mask = RoundItems - 1;

    HashStore.Iteration = 0;

    HashItemPointer = (HashItem*)realloc(HashStore.Item, HashStore.Size);

    if (HashItemPointer == NULL) { // Allocate memory error
        printf("Allocate memory to hash table error!\n");

        return FALSE;
    }

    HashStore.Item = HashItemPointer;

    return TRUE;
}

void InitHashBoards(void)
{
//    printf("HashDataS = %zd HashDataU = %zd HashItem = %zd\n", sizeof(HashDataS), sizeof(HashDataU), sizeof(HashItem));

    for (int Color = 0; Color < 2; ++Color) {
        for (int Piece = 0; Piece < 6; ++Piece) {
            for (int Square = 0; Square < 64; ++Square) {
                PieceHash[Color][Piece][Square] = Rand64();
            }
        }
    }

    ColorHash = Rand64();

    for (int Square = 0; Square < 64; ++Square) {
        PassantHash[Square] = Rand64();
    }
}

void InitHash(BoardItem* Board)
{
    U64 Pieces;

    int Square;

    U64 Hash = 0ULL;

    // White pieces

    Pieces = Board->BB_WhitePieces;

    while (Pieces) {
        Square = LSB(Pieces);

        Hash ^= PieceHash[WHITE][PIECE_TYPE(Board->Pieces[Square])][Square];

        Pieces &= Pieces - 1;
    }

    // Black pieces

    Pieces = Board->BB_BlackPieces;

    while (Pieces) {
        Square = LSB(Pieces);

        Hash ^= PieceHash[BLACK][PIECE_TYPE(Board->Pieces[Square])][Square];

        Pieces &= Pieces - 1;
    }

    // Color

    if (Board->CurrentColor == BLACK) {
        Hash ^= ColorHash;
    }

    // En passant

    if (Board->PassantSquare != -1) {
        Hash ^= PassantHash[Board->PassantSquare];
    }

    Board->Hash = Hash;
}

void ClearHash(void)
{
    HashStore.Iteration = 0;

    memset(HashStore.Item, 0, HashStore.Size);
}

void AddHashStoreIteration(void)
{
    HashStore.Iteration = (HashStore.Iteration + (U8)1) & (U8)15; // 4 bits
}

void SaveHash(const U64 Hash, const int Depth, const int Ply, const int Score, const int StaticScore, const int Move, const int Flag)
{
    HashItem* HashItemPointer = &HashStore.Item[Hash & HashStore.Mask];

    HashDataU DataU = HashItemPointer->Value; // Load data from record

    if (
        (HashItemPointer->KeyValue ^ Hash) == DataU.RawData
        || Depth >= DataU.Data.Depth
        || DataU.Data.Iteration != HashStore.Iteration
    ) { // Xiphos
        // Replace record

        // Adjust the score
        if (Score < -INF + MAX_PLY) {
            DataU.Data.Score = (I16)(Score - Ply);
        }
        else if (Score > INF - MAX_PLY) {
            DataU.Data.Score = (I16)(Score + Ply);
        }
        else {
            DataU.Data.Score = (I16)Score;
        }

        DataU.Data.StaticScore = (I16)StaticScore;
        DataU.Data.Move = (U16)Move;
        DataU.Data.Depth = (I8)Depth;
        DataU.Data.Flag = (U8)Flag;
        DataU.Data.Iteration = HashStore.Iteration;

        // Save record
        HashItemPointer->KeyValue = (Hash ^ DataU.RawData);
        HashItemPointer->Value = DataU;
    }
}

void LoadHash(const U64 Hash, int* Depth, const int Ply, int* Score, int* StaticScore, int* Move, int* Flag)
{
    HashItem* HashItemPointer = &HashStore.Item[Hash & HashStore.Mask];

    HashDataU DataU = HashItemPointer->Value; // Load data from record

    if ((HashItemPointer->KeyValue ^ Hash) != DataU.RawData) { // Hash does not match or data is corrupted (SMP)
        return;
    }

    // Adjust the score
    if (DataU.Data.Score < -INF + MAX_PLY) {
        *Score = DataU.Data.Score + Ply;
    }
    else if (DataU.Data.Score > INF - MAX_PLY) {
        *Score = DataU.Data.Score - Ply;
    }
    else {
        *Score = DataU.Data.Score;
    }

    *StaticScore = DataU.Data.StaticScore;
    *Move = DataU.Data.Move;
    *Depth = DataU.Data.Depth;
    *Flag = DataU.Data.Flag;
}

int FullHash(void)
{
    int HashHit = 0;

    for (int Index = 0; Index < 1000; ++Index) {
        if (HashStore.Item[Index].Value.Data.Iteration == HashStore.Iteration) {
            ++HashHit;
        }
    }

    return HashHit; // In per mille (0.1%)
}

#ifdef HASH_PREFETCH
void Prefetch(const U64 Hash)
{
    HashItem* HashItemPointer = &HashStore.Item[Hash & HashStore.Mask];

    _mm_prefetch((char*)HashItemPointer, _MM_HINT_T0);
}
#endif // HASH_PREFETCH