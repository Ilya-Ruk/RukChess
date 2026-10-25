// BitBoard.cpp

#include "stdafx.h"

#include "BitBoard.h"

#include "Board.h"
#include "Def.h"
#include "Types.h"

// Board 12x10 with the flag of going beyond the board

const int Board120[120] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7, -1,
    -1,  8,  9, 10, 11, 12, 13, 14, 15, -1,
    -1, 16, 17, 18, 19, 20, 21, 22, 23, -1,
    -1, 24, 25, 26, 27, 28, 29, 30, 31, -1,
    -1, 32, 33, 34, 35, 36, 37, 38, 39, -1,
    -1, 40, 41, 42, 43, 44, 45, 46, 47, -1,
    -1, 48, 49, 50, 51, 52, 53, 54, 55, -1,
    -1, 56, 57, 58, 59, 60, 61, 62, 63, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

// 8x8 board index conversion array into 12x10 board index

const int Board64[64] = {
    21, 22, 23, 24, 25, 26, 27, 28,
    31, 32, 33, 34, 35, 36, 37, 38,
    41, 42, 43, 44, 45, 46, 47, 48,
    51, 52, 53, 54, 55, 56, 57, 58,
    61, 62, 63, 64, 65, 66, 67, 68,
    71, 72, 73, 74, 75, 76, 77, 78,
    81, 82, 83, 84, 85, 86, 87, 88,
    91, 92, 93, 94, 95, 96, 97, 98
};

// Shift of pieces on a board 12x10

const int MoveDelta[6][8] = {
    {   0,   0,   0,   0,   0,   0,   0,   0 }, // Pawn (not used)
    { -21, -19, -12,  -8,   8,  12,  19,  21 }, // Knight
    { -11,  -9,   9,  11,   0,   0,   0,   0 }, // Bishop
    { -10,  -1,   1,  10,   0,   0,   0,   0 }, // Rook
    { -11, -10,  -9,  -1,   1,   9,  10,  11 }, // Queen
    { -11, -10,  -9,  -1,   1,   9,  10,  11 }  // King
};

U64 BB_KnightAttack[64];
U64 BB_KingAttack[64];

U64 BB_BishopMask[64];
U64 BB_RookMask[64];

int BB_BishopOffset[64];
int BB_RookOffset[64];

U64 BB_BishopAttack[5248];
U64 BB_RookAttack[102400];

int POPCNT(const U64 Source)
{
    return (int)__popcnt64(Source);
}

int LSB(const U64 Source)
{
    unsigned long Index;

    _BitScanForward64(&Index, Source);

    return (int)Index;
}
/*
int MSB(const U64 Source)
{
    unsigned long Index;

    _BitScanReverse64(&Index, Source);

    return (int)Index;
}
*/
U64 PDEP(const U64 Source, const U64 Mask)
{
    return _pdep_u64(Source, Mask);
}

int PEXT(const U64 Source, const U64 Mask)
{
    return (int)_pext_u64(Source, Mask);
}

U64 CalculatePieceAttack(const int Piece, const int Square, const U64 Occupied, const BOOL Truncate)
{
    int To;

    U64 Bit;
    U64 PreviousBit;

    U64 Result = 0ULL;

    for (int Direction = 0; Direction < 8; ++Direction) {
        if (!MoveDelta[Piece][Direction]) { // Out of directions
            break; // for
        }

        To = Square;

        PreviousBit = 0ULL;

        while (TRUE) {
            To = Board120[Board64[To] + MoveDelta[Piece][Direction]];

            if (To == -1) { // Out of board
                if (Truncate) {
                    Result &= ~PreviousBit;
                }

                break; // while
            }

            Bit = BB_SQUARE(To);

            Result |= Bit;

            if (Piece == KING || Piece == KNIGHT) {
                break; // while
            }

            if (Bit & Occupied) {
                break; // while
            }

            PreviousBit = Bit;
        }
    }

    return Result;
}

void InitAttackTable(const int Piece, const int Square, const U64* BB_Mask, int* BB_OffsetTable, U64* BB_AttackTable, int* Offset)
{
    U64 Occupied;

    U64 Attack;

#ifdef DEBUG_BIT_BOARD_INIT
    U64 PreviousAttack;
#endif // DEBUG_BIT_BOARD_INIT

    int CountBits = POPCNT(BB_Mask[Square]); // Max. 12 bits (example, rook on a1)

    int MaxIndex = 1 << CountBits; // Max. 4096 (1 << 12)

    BB_OffsetTable[Square] = *Offset;

    for (int Index = 0; Index < MaxIndex; ++Index) {
        Occupied = PDEP((U64)Index, BB_Mask[Square]);

        Attack = CalculatePieceAttack(Piece, Square, Occupied, FALSE);

#ifdef DEBUG_BIT_BOARD_INIT
        PreviousAttack = BB_AttackTable[*Offset + Index];

        if (PreviousAttack && PreviousAttack != Attack) {
            if (Piece == BISHOP) {
                printf("-- Bishop attack error!\n");
            }
            else { // Rook
                printf("-- Rook attack error!\n");
            }
        }
#endif // DEBUG_BIT_BOARD_INIT

        BB_AttackTable[*Offset + Index] = Attack;
    }

    *Offset += MaxIndex;
}

void InitBitBoards(void)
{
    int BishopOffset = 0;
    int RookOffset = 0;

    // Init Bishop and Rook mask tables
    for (int Square = 0; Square < 64; ++Square) {
        BB_BishopMask[Square] = CalculatePieceAttack(BISHOP, Square, 0ULL, TRUE);
        BB_RookMask[Square] = CalculatePieceAttack(ROOK, Square, 0ULL, TRUE);
    }

    // Init Knight and King attack tables; Bishop and Rook offset and attack tables
    for (int Square = 0; Square < 64; ++Square) {
        BB_KnightAttack[Square] = CalculatePieceAttack(KNIGHT, Square, 0ULL, FALSE);
        BB_KingAttack[Square] = CalculatePieceAttack(KING, Square, 0ULL, FALSE);

        InitAttackTable(BISHOP, Square, BB_BishopMask, BB_BishopOffset, BB_BishopAttack, &BishopOffset);
        InitAttackTable(ROOK, Square, BB_RookMask, BB_RookOffset, BB_RookAttack, &RookOffset);
    }
}

U64 PawnAttacks(const U64 Pawns, const int Color)
{
    if (Color == WHITE) {
        return ((Pawns >> 7) & ~BB_FILE_A) | ((Pawns >> 9) & ~BB_FILE_H);
    }
    else { // BLACK
        return ((Pawns << 9) & ~BB_FILE_A) | ((Pawns << 7) & ~BB_FILE_H);
    }
}

U64 PushedPawns(const U64 Pawns, const int Color, const U64 NotOccupied)
{
    if (Color == WHITE) {
        return (Pawns >> 8) & NotOccupied;
    }
    else { // BLACK
        return (Pawns << 8) & NotOccupied;
    }
}

U64 PushedPawns2(const U64 Pawns, const int Color, const U64 NotOccupied)
{
    if (Color == WHITE) {
        return (((Pawns >> 8) & NotOccupied) >> 8) & NotOccupied & BB_RANK_4;
    }
    else { // BLACK
        return (((Pawns << 8) & NotOccupied) << 8) & NotOccupied & BB_RANK_5;
    }
}

U64 KnightAttacks(const int Square)
{
    return BB_KnightAttack[Square];
}

U64 BishopAttacks(const int Square, const U64 Occupied)
{
    U64 BishopOccupied = Occupied & BB_BishopMask[Square];

    int Offset = BB_BishopOffset[Square];

    int Index = PEXT(BishopOccupied, BB_BishopMask[Square]);

    return BB_BishopAttack[Offset + Index];
}

U64 RookAttacks(const int Square, const U64 Occupied)
{
    U64 RookOccupied = Occupied & BB_RookMask[Square];

    int Offset = BB_RookOffset[Square];

    int Index = PEXT(RookOccupied, BB_RookMask[Square]);

    return BB_RookAttack[Offset + Index];
}

U64 QueenAttacks(const int Square, const U64 Occupied)
{
    return (BishopAttacks(Square, Occupied) | RookAttacks(Square, Occupied));
}

U64 KingAttacks(const int Square)
{
    return BB_KingAttack[Square];
}