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
    >>>>>> _BitScanReverse64() >>>>>>
    63 62 61 60 59 58 ... 5 4 3 2 1 0
    (MSB)                       (LSB)
    (h1)                         (a8)
*/

// Piece type and color: 4 bits = Piece color (1 bit) + Piece type (3 bits)

// Piece type (3 bits)

#define PAWN    0
#define KNIGHT  1
#define BISHOP  2
#define ROOK    3
#define QUEEN   4
#define KING    5

// Piece color (1 bit)

#define WHITE   0
#define BLACK   1

// Empty square

#define EMPTY   15

#define PIECE(PieceAndColor)                    ((PieceAndColor) & 7)
#define COLOR(PieceAndColor)                    ((PieceAndColor) >> 3)

#define PIECE_AND_COLOR(Piece, Color)           (((Color) << 3) | (Piece))

#define CHANGE_COLOR(Color)                     ((Color) ^ 1)

#define FILE(Square)                            ((Square) & 7)
#define RANK(Square)                            ((Square) >> 3)

#define SQUARE(Rank, File)                      (((Rank) << 3) | (File))

// Move: 16 bits = Promote piece (4 bits) + From (6 bits) + To (6 bits)

#define MOVE_PROMOTE_PIECE(Move)                ((Move) >> 12)
#define MOVE_FROM(Move)                         (((Move) >> 6) & 63)
#define MOVE_TO(Move)                           ((Move) & 63)

#define MOVE_CREATE(From, To, PromotePiece)     (((PromotePiece) << 12) | ((From) << 6) | (To))

// Move type flags

#define MOVE_QUIET              0   // Quiet move (not pawn)

#define MOVE_PAWN               1   // Pawn move (capture or pushed move)
#define MOVE_PAWN_2             2   // Pawn move (pused move 2)
#define MOVE_PAWN_PASSANT       4   // Pawn move (en passant)
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

typedef struct {
    int Type;
    int Move; // (PromotePiece << 12) | (From << 6) | To
    int SortValue;
} MoveItem; // 12 bytes

#ifdef NNUE_EVALUATION_FUNCTION
typedef struct {
    _declspec(align(64)) I16 Accumulation[2][256]; // [Perspective][Half feature output dimension]

    BOOL AccumulationComputed;
} AccumulatorItem; // 1028 bytes (aligned 1088 bytes)
#endif // NNUE_EVALUATION_FUNCTION

#ifdef NNUE_EVALUATION_FUNCTION_2
typedef struct {
    _declspec(align(64)) I16 Accumulation[2][256]; // [Perspective][Hidden dimension]

    BOOL AccumulationComputed;
} AccumulatorItem; // 2052 bytes (aligned 2080 bytes)
#endif // NNUE_EVALUATION_FUNCTION_2

typedef struct {
    int Type;

    int From;
    int PieceFrom;

    int To;
    int PieceTo;

    int PromotePiece;

    int PassantSquare;
    int EatPawnSquare;

    int CastleFlags;
    int FiftyMove;

    U64 Hash;

#ifdef DEBUG_MOVE
    U64 BB_WhitePieces;
    U64 BB_BlackPieces;
    U64 BB_Pieces[2][6]; // [Color][Piece]
#endif // DEBUG_MOVE

#if defined(NNUE_EVALUATION_FUNCTION) || defined(NNUE_EVALUATION_FUNCTION_2)
    AccumulatorItem Accumulator;
#endif // NNUE_EVALUATION_FUNCTION || NNUE_EVALUATION_FUNCTION_2
} HistoryItem; // 48 bytes/1152 bytes (NNUE)/2144 (NNUE2)

typedef struct {
    int Pieces[64]; // (Color << 3) | Piece
    int CurrentColor;
    int CastleFlags;
    int PassantSquare;
    int FiftyMove;
    int HalfMoveNumber;

    U64 BB_WhitePieces;
    U64 BB_BlackPieces;
    U64 BB_Pieces[2][6]; // [Color][Piece]

    U64 Hash;

    HistoryItem MoveTable[MAX_GAME_MOVES]; // 1024 moves x 48 bytes/1152 bytes (NNUE)/2144 (NNUE2) = 49152 bytes/1179648 bytes (NNUE)/2195456 (NNUE2)

#if defined(PVS) || defined(QUIESCENCE_PVS)
    BOOL FollowPV;
#endif // PVS || QUIESCENCE_PVS

    U64 Nodes;

#ifdef DEBUG_STATISTIC
    U64 HashCount;
    U64 EvaluateCount;
    U64 CutoffCount;
    U64 QuiescenceCount;
#endif // DEBUG_STATISTIC

    int SelDepth;

    MoveItem BestMovesRoot[MAX_PLY]; // 128 plys x 12 bytes = 1536 bytes

#ifndef COMMON_HEURISTIC_TABLE
    int HeuristicTable[2][6][64]; // [Color][Piece][Square] // 2 colors x 6 pieces x 64 squares x 4 bytes = 3072 bytes
#endif // !COMMON_HEURISTIC_TABLE

#ifndef COMMON_KILLER_MOVE_TABLE
    int KillerMoveTable[MAX_PLY + 1][2]; // [Max. ply + 1][Two killer moves] // (128 + 1) plys x 2 moves x 4 bytes = 1032 bytes
#endif // !COMMON_KILLER_MOVE_TABLE

#ifndef COMMON_COUNTER_MOVE_TABLE
    int CounterMoveTable[2][6][64]; // [Color][Piece][Square] // 2 colors x 6 pieces x 64 squares x 4 bytes = 3072 bytes
#endif // !COMMON_COUNTER_MOVE_TABLE

#if defined(NNUE_EVALUATION_FUNCTION) || defined(NNUE_EVALUATION_FUNCTION_2)
    AccumulatorItem Accumulator; // 1028 bytes (aligned 1088 bytes)/2052 bytes (aligned 2080 bytes)
#endif // NNUE_EVALUATION_FUNCTION || NNUE_EVALUATION_FUNCTION_2
} BoardItem; // 58280 bytes/1189920 bytes (NNUE)/2206688 (NNUE2)

extern const char* BoardName[64];

extern const char PiecesCharWhite[6];
extern const char PiecesCharBlack[6];

extern const int CastleMask[64];

extern const char MoveFirstChar[];
extern const char MoveSubsequentChar[];

extern char StartFen[];

extern BoardItem CurrentBoard;

#ifdef LATE_MOVE_PRUNING
extern int LateMovePruningTable[7]; // [LMP depth]
#endif // LATE_MOVE_PRUNING

#if defined(NEGA_SCOUT) && defined(LATE_MOVE_REDUCTION)
extern int LateMoveReductionTable[64][64]; // [LMR depth][Move number]
#endif // NEGA_SCOUT && LATE_MOVE_REDUCTION

BOOL IsSquareAttacked(const BoardItem* Board, const int Square, const int Color);
BOOL IsInCheck(const BoardItem* Board, const int Color);

BOOL HasLegalMoves(BoardItem* Board);

void NotateMove(BoardItem* Board, const MoveItem Move, char* Result);

int PositionRepeat1(const BoardItem* Board);
int PositionRepeat2(const BoardItem* Board);

BOOL IsInsufficientMaterial(const BoardItem* Board);

void PrintBoard(BoardItem* Board);

void PrintBitMask(const U64 Mask);

int SetFen(BoardItem* Board, char* Fen);
void GetFen(const BoardItem* Board, char* Fen);

U64 CountLegalMoves(const int Depth);

#endif // !BOARD_H