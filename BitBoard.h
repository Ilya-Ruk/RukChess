// BitBoard.h

#pragma once

#ifndef BITBOARD_H
#define BITBOARD_H

#include "Def.h"
#include "Types.h"

// Files masks

#define BB_FILE_A   0x0101010101010101ULL
#define BB_FILE_B   0x0202020202020202ULL
#define BB_FILE_C   0x0404040404040404ULL
#define BB_FILE_D   0x0808080808080808ULL
#define BB_FILE_E   0x1010101010101010ULL
#define BB_FILE_F   0x2020202020202020ULL
#define BB_FILE_G   0x4040404040404040ULL
#define BB_FILE_H   0x8080808080808080ULL

// Ranks masks

#define BB_RANK_1   0xFF00000000000000ULL
#define BB_RANK_2   0x00FF000000000000ULL
#define BB_RANK_3   0x0000FF0000000000ULL
#define BB_RANK_4   0x000000FF00000000ULL
#define BB_RANK_5   0x00000000FF000000ULL
#define BB_RANK_6   0x0000000000FF0000ULL
#define BB_RANK_7   0x000000000000FF00ULL
#define BB_RANK_8   0x00000000000000FFULL

// Squares masks

#define BB_A1       (BB_FILE_A & BB_RANK_1)
#define BB_A2       (BB_FILE_A & BB_RANK_2)
#define BB_A3       (BB_FILE_A & BB_RANK_3)
#define BB_A4       (BB_FILE_A & BB_RANK_4)
#define BB_A5       (BB_FILE_A & BB_RANK_5)
#define BB_A6       (BB_FILE_A & BB_RANK_6)
#define BB_A7       (BB_FILE_A & BB_RANK_7)
#define BB_A8       (BB_FILE_A & BB_RANK_8)

#define BB_B1       (BB_FILE_B & BB_RANK_1)
#define BB_B2       (BB_FILE_B & BB_RANK_2)
#define BB_B3       (BB_FILE_B & BB_RANK_3)
#define BB_B4       (BB_FILE_B & BB_RANK_4)
#define BB_B5       (BB_FILE_B & BB_RANK_5)
#define BB_B6       (BB_FILE_B & BB_RANK_6)
#define BB_B7       (BB_FILE_B & BB_RANK_7)
#define BB_B8       (BB_FILE_B & BB_RANK_8)

#define BB_C1       (BB_FILE_C & BB_RANK_1)
#define BB_C2       (BB_FILE_C & BB_RANK_2)
#define BB_C3       (BB_FILE_C & BB_RANK_3)
#define BB_C4       (BB_FILE_C & BB_RANK_4)
#define BB_C5       (BB_FILE_C & BB_RANK_5)
#define BB_C6       (BB_FILE_C & BB_RANK_6)
#define BB_C7       (BB_FILE_C & BB_RANK_7)
#define BB_C8       (BB_FILE_C & BB_RANK_8)

#define BB_D1       (BB_FILE_D & BB_RANK_1)
#define BB_D2       (BB_FILE_D & BB_RANK_2)
#define BB_D3       (BB_FILE_D & BB_RANK_3)
#define BB_D4       (BB_FILE_D & BB_RANK_4)
#define BB_D5       (BB_FILE_D & BB_RANK_5)
#define BB_D6       (BB_FILE_D & BB_RANK_6)
#define BB_D7       (BB_FILE_D & BB_RANK_7)
#define BB_D8       (BB_FILE_D & BB_RANK_8)

#define BB_E1       (BB_FILE_E & BB_RANK_1)
#define BB_E2       (BB_FILE_E & BB_RANK_2)
#define BB_E3       (BB_FILE_E & BB_RANK_3)
#define BB_E4       (BB_FILE_E & BB_RANK_4)
#define BB_E5       (BB_FILE_E & BB_RANK_5)
#define BB_E6       (BB_FILE_E & BB_RANK_6)
#define BB_E7       (BB_FILE_E & BB_RANK_7)
#define BB_E8       (BB_FILE_E & BB_RANK_8)

#define BB_F1       (BB_FILE_F & BB_RANK_1)
#define BB_F2       (BB_FILE_F & BB_RANK_2)
#define BB_F3       (BB_FILE_F & BB_RANK_3)
#define BB_F4       (BB_FILE_F & BB_RANK_4)
#define BB_F5       (BB_FILE_F & BB_RANK_5)
#define BB_F6       (BB_FILE_F & BB_RANK_6)
#define BB_F7       (BB_FILE_F & BB_RANK_7)
#define BB_F8       (BB_FILE_F & BB_RANK_8)

#define BB_G1       (BB_FILE_G & BB_RANK_1)
#define BB_G2       (BB_FILE_G & BB_RANK_2)
#define BB_G3       (BB_FILE_G & BB_RANK_3)
#define BB_G4       (BB_FILE_G & BB_RANK_4)
#define BB_G5       (BB_FILE_G & BB_RANK_5)
#define BB_G6       (BB_FILE_G & BB_RANK_6)
#define BB_G7       (BB_FILE_G & BB_RANK_7)
#define BB_G8       (BB_FILE_G & BB_RANK_8)

#define BB_H1       (BB_FILE_H & BB_RANK_1)
#define BB_H2       (BB_FILE_H & BB_RANK_2)
#define BB_H3       (BB_FILE_H & BB_RANK_3)
#define BB_H4       (BB_FILE_H & BB_RANK_4)
#define BB_H5       (BB_FILE_H & BB_RANK_5)
#define BB_H6       (BB_FILE_H & BB_RANK_6)
#define BB_H7       (BB_FILE_H & BB_RANK_7)
#define BB_H8       (BB_FILE_H & BB_RANK_8)

// Mask by square

#define BB_SQUARE(Square)   (1ULL << (Square))

int POPCNT(const U64 Source);

int LSB(const U64 Source);
//int MSB(const U64 Source);

void InitBitBoards(void);

U64 PawnAttacks(const U64 Pawns, const int Color);
U64 PushedPawns(const U64 Pawns, const int Color, const U64 NotOccupied);
U64 PushedPawns2(const U64 Pawns, const int Color, const U64 NotOccupied);
U64 KnightAttacks(const int Square);
U64 BishopAttacks(const int Square, const U64 Occupied);
U64 RookAttacks(const int Square, const U64 Occupied);
U64 QueenAttacks(const int Square, const U64 Occupied);
U64 KingAttacks(const int Square);

#endif // !BITBOARD_H