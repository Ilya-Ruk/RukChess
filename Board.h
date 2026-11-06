// Board.h

#pragma once

#ifndef BOARD_H
#define BOARD_H

#include "Def.h"
#include "Types.h"

/*
    *************************** (BB Rank) (Rank)
    * a8 b8 c8 d8 e8 f8 g8 h8 *    8        0
    * a7 b7 c7 d7 e7 f7 g7 h7 *    7        1
    * a6 b6 c6 d6 e6 f6 g6 h6 *    6        2
    * a5 b5 c5 d5 e5 f5 g5 h5 *    5        3
    * a4 b4 c4 d4 e4 f4 g4 h4 *    4        4
    * a3 b3 c3 d3 e3 f3 g3 h3 *    3        5
    * a2 b2 c2 d2 e2 f2 g2 h2 *    2        6
    * a1 b1 c1 d1 e1 f1 g1 h1 *    1        7
    ***************************
       A  B  C  D  E  F  G  H (BB File)
       0  1  2  3  4  5  6  7 (File)

    *************************** (BB Rank) (Rank)
    *  0  1  2  3  4  5  6  7 *    8        0
    *  8  9 10 11 12 13 14 15 *    7        1
    * 16 17 18 19 20 21 22 23 *    6        2
    * 24 25 26 27 28 29 30 31 *    5        3
    * 32 33 34 35 36 37 38 39 *    4        4
    * 40 41 42 43 44 45 46 47 *    3        5
    * 48 49 50 51 52 53 54 55 *    2        6
    * 56 57 58 59 60 61 62 63 *    1        7
    ***************************
       A  B  C  D  E  F  G  H (BB File)
       0  1  2  3  4  5  6  7 (File)

    <<<<<< _BitScanForward64() <<<<<<
    63 62 61 60 59 58 ... 5 4 3 2 1 0
    (MSB)                       (LSB)
    (h1)                         (a8)
*/

// Piece: 4 bits = Piece color (1 bit) + Piece type (3 bits)

// Piece type (3 bits)

#define PAWN                    0
#define KNIGHT                  1
#define BISHOP                  2
#define ROOK                    3
#define QUEEN                   4
#define KING                    5

// Piece color (1 bit)

#define WHITE                   0
#define BLACK                   1

#define NO_PIECE                15

#define PIECE_TYPE(Piece)                           ((Piece) & 7)
#define PIECE_COLOR(Piece)                          ((Piece) >> 3)

#define PIECE_CREATE(PieceType, PieceColor)         (((PieceColor) << 3) | (PieceType))

#define CHANGE_COLOR(Color)                         ((Color) ^ 1)

// Square of board: 6 bits = Rank (3 bits) + File (3 bits)

#define FILE(Square)                                ((Square) & 7)
#define RANK(Square)                                ((Square) >> 3)

#define SQUARE_CREATE(File, Rank)                   (((Rank) << 3) | (File))

// Move: 15 bits = Promote piece type (3 bits) + From (6 bits) + To (6 bits)

#define MOVE_FROM(Move)                             (((Move) >> 6) & 63)
#define MOVE_TO(Move)                               ((Move) & 63)
#define MOVE_PROMOTE_PIECE_TYPE(Move)               ((Move) >> 12)

#define MOVE_CREATE(From, To, PromotePieceType)     (((PromotePieceType) << 12) | ((From) << 6) | (To))

// Move type flags

#define MOVE_QUIET              0   // Quiet move (not pawn)

#define MOVE_PAWN               1   // Pawn move (capture or pushed move)
#define MOVE_PAWN_2             2   // Pawn move (pused move 2)
#define MOVE_PAWN_EN_PASSANT    4   // Pawn move (en passant)
#define MOVE_PAWN_PROMOTE       8   // Pawn move (promote)

#define MOVE_CAPTURE            16  // Capture move

#define MOVE_CASTLE_KING        32  // O-O
#define MOVE_CASTLE_QUEEN       64  // O-O-O

#define MOVE_NULL               128 // Null move

// Castle permission flags (part 1)

#define CASTLE_WHITE_KING       1   // White O-O
#define CASTLE_WHITE_QUEEN      2   // White O-O-O
#define CASTLE_BLACK_KING       4   // Black O-O
#define CASTLE_BLACK_QUEEN      8   // Black O-O-O

// Squares index

enum {
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1
};

typedef struct {
    int Type;
    int Move; // (PromotePieceType << 12) | (From << 6) | To
    int SortValue;
} MoveItem;

typedef struct {
    _declspec(align(64)) I16 Accumulator[2][HIDDEN_DIMENSION]; // [Perspective][Hidden dimension]
} AccumulatorItem;

typedef struct {
    int Type;

    int From;
    int PieceTypeFrom;

    int To;
    int PieceTypeTo;

    int PromotePieceType;

    int EnPassantSquare;
    int CapturePawnSquare;

    int CastleFlags;
    int FiftyMove;

    U64 Hash;

#ifdef DEBUG_MOVE_BIT_BOARD
    U64 BB_WhitePieces;
    U64 BB_BlackPieces;
    U64 BB_Pieces[2][6]; // [Color][Piece]
#endif // DEBUG_MOVE_BIT_BOARD
} HistoryItem;

typedef struct {
    int Pieces[64]; // (Color << 3) | Piece
    int CurrentColor;
    int CastleFlags;
    int EnPassantSquare;
    int FiftyMove;
    int HalfMoveNumber;

    U64 BB_WhitePieces;
    U64 BB_BlackPieces;
    U64 BB_Pieces[2][6]; // [Color][Piece]

    U64 Hash;

    HistoryItem MoveTable[MAX_GAME_MOVES];

    AccumulatorItem Accumulator;
} BoardItem;

typedef struct {
    U64 Nodes;

#ifdef USE_STATISTIC
    U64 HashCount;
    U64 EvaluateCount;
    U64 CutoffCount;
    U64 QuiescenceCount;
#endif // USE_STATISTIC

    int SelDepth;

    MoveItem BestMoves[MAX_PLY];

    int HeuristicTable[2][6][64]; // [Color][Piece][Square]

#ifdef COUNTER_MOVE_HISTORY
    int CounterMoveHistoryTable[6][64][6 * 64]; // [Piece][Square][Piece * Square]
#endif // COUNTER_MOVE_HISTORY

#ifdef KILLER_MOVE
    int KillerMoveTable[MAX_PLY + 1][2]; // [Max. ply + 1][Two killer moves]
#endif // KILLER_MOVE

#ifdef COUNTER_MOVE
    int CounterMoveTable[2][6][64]; // [Color][Piece][Square]
#endif // COUNTER_MOVE
} SearchDataItem;

extern const char* BoardName[64];

extern const char PiecesCharWhite[6];
extern const char PiecesCharBlack[6];

extern const int CastleMask[64];

extern const char MoveFirstChar[];
extern const char MoveSubsequentChar[];

extern const char StartFen[];

extern BoardItem BoardList[MAX_THREADS];
extern SearchDataItem SearchDataList[MAX_THREADS];

#ifdef LATE_MOVE_PRUNING
extern int LateMovePruningTable[7]; // [LMP depth]
#endif // LATE_MOVE_PRUNING

#ifdef LATE_MOVE_REDUCTION
extern int LateMoveReductionTable[64][64]; // [LMR depth][Move number]
#endif // LATE_MOVE_REDUCTION

BOOL IsSquareAttacked(const BoardItem* Board, const int Square, const int Color);
BOOL IsInCheck(const BoardItem* Board, const int Color);

BOOL HasLegalMoves(BoardItem* Board);

void NotateMove(BoardItem* Board, const MoveItem Move, char* Result);

int PositionRepeat1(const BoardItem* Board);
int PositionRepeat2(const BoardItem* Board);

BOOL IsInsufficientMaterial(const BoardItem* Board);

void PrintBoard(const BoardItem* Board);

int SetFen(BoardItem* Board, const char* Fen);
void GetFen(const BoardItem* Board, char* Fen);

U64 CountLegalMoves(BoardItem* Board, const int Depth);

#endif // !BOARD_H