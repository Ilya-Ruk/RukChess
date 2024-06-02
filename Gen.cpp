// Gen.cpp

#include "stdafx.h"

#include "Gen.h"

#include "BitBoard.h"
#include "Board.h"
#include "Def.h"
#include "Heuristic.h"
#include "Move.h"
#include "Sort.h"
#include "Types.h"

void AddMove(const BoardItem* Board, int** CMH_Pointer, MoveItem* MoveList, int* GenMoveCount, const int From, const int To, const int MoveType)
{
    if ((MoveType & MOVE_PAWN) && (RANK(To) == 0 || RANK(To) == 7)) { // Pawn promote
        // Queen
        MoveList[*GenMoveCount].Type = (MoveType | MOVE_PAWN_PROMOTE);
        MoveList[*GenMoveCount].Move = MOVE_CREATE(From, To, QUEEN);
        MoveList[*GenMoveCount].SortValue = SORT_PAWN_PROMOTE_MOVE_BONUS + QUEEN;

        ++(*GenMoveCount);

        // Rook
        MoveList[*GenMoveCount].Type = (MoveType | MOVE_PAWN_PROMOTE);
        MoveList[*GenMoveCount].Move = MOVE_CREATE(From, To, ROOK);
        MoveList[*GenMoveCount].SortValue = SORT_PAWN_PROMOTE_MOVE_BONUS + ROOK;

        ++(*GenMoveCount);

        // Bishop
        MoveList[*GenMoveCount].Type = (MoveType | MOVE_PAWN_PROMOTE);
        MoveList[*GenMoveCount].Move = MOVE_CREATE(From, To, BISHOP);
        MoveList[*GenMoveCount].SortValue = SORT_PAWN_PROMOTE_MOVE_BONUS + BISHOP;

        ++(*GenMoveCount);

        // Knight
        MoveList[*GenMoveCount].Type = (MoveType | MOVE_PAWN_PROMOTE);
        MoveList[*GenMoveCount].Move = MOVE_CREATE(From, To, KNIGHT);
        MoveList[*GenMoveCount].SortValue = SORT_PAWN_PROMOTE_MOVE_BONUS + KNIGHT;

        ++(*GenMoveCount);
    }
    else {
        MoveList[*GenMoveCount].Type = MoveType;
        MoveList[*GenMoveCount].Move = MOVE_CREATE(From, To, 0);

        if (MoveType & MOVE_CAPTURE) {
            if (MoveType & MOVE_PAWN_PASSANT) {
                MoveList[*GenMoveCount].SortValue = SORT_CAPTURE_MOVE_BONUS + ((PAWN + 1) << 3) - (PAWN + 1);
            }
            else {
                MoveList[*GenMoveCount].SortValue = SORT_CAPTURE_MOVE_BONUS + ((PIECE(Board->Pieces[To]) + 1) << 3) - (PIECE(Board->Pieces[From]) + 1);
            }
        }
        else {
            MoveList[*GenMoveCount].SortValue = Board->HeuristicTable[Board->CurrentColor][PIECE(Board->Pieces[From])][To];

#ifdef COUNTER_MOVE_HISTORY
            if (CMH_Pointer) {
                if (CMH_Pointer[0]) {
                    MoveList[*GenMoveCount].SortValue += CMH_Pointer[0][(PIECE(Board->Pieces[From]) << 6) + To];
                }

                if (CMH_Pointer[1]) {
                    MoveList[*GenMoveCount].SortValue += CMH_Pointer[1][(PIECE(Board->Pieces[From]) << 6) + To];
                }
            }
#endif // COUNTER_MOVE_HISTORY
        }

        ++(*GenMoveCount);
    }
}

void GenerateAllMoves(const BoardItem* Board, int** CMH_Pointer, MoveItem* MoveList, int* GenMoveCount)
{
    U64 Pieces;
    U64 Attacks;

    U64 CaptureMoves;
    U64 QuietMoves;

    int From;
    int To;

    // Pawns captures
    if (Board->CurrentColor == WHITE) {
        Attacks = PawnAttacks(Board->BB_Pieces[WHITE][PAWN], WHITE) & Board->BB_BlackPieces;

        while (Attacks) {
            To = LSB(Attacks);

            if (((BB_SQUARE(To) << 7) & ~BB_FILE_H) & Board->BB_Pieces[WHITE][PAWN]) {
                AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To + 7, To, (MOVE_PAWN | MOVE_CAPTURE));
            }

            if (((BB_SQUARE(To) << 9) & ~BB_FILE_A) & Board->BB_Pieces[WHITE][PAWN]) {
                AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To + 9, To, (MOVE_PAWN | MOVE_CAPTURE));
            }

            Attacks &= Attacks - 1;
        }
    }
    else { // BLACK
        Attacks = PawnAttacks(Board->BB_Pieces[BLACK][PAWN], BLACK) & Board->BB_WhitePieces;

        while (Attacks) {
            To = LSB(Attacks);

            if (((BB_SQUARE(To) >> 7) & ~BB_FILE_A)& Board->BB_Pieces[BLACK][PAWN]) {
                AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To - 7, To, (MOVE_PAWN | MOVE_CAPTURE));
            }

            if (((BB_SQUARE(To) >> 9) & ~BB_FILE_H)& Board->BB_Pieces[BLACK][PAWN]) {
                AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To - 9, To, (MOVE_PAWN | MOVE_CAPTURE));
            }

            Attacks &= Attacks - 1;
        }
    }

    // Pawns captures (en passant)
    if (Board->PassantSquare != -1) {
        To = Board->PassantSquare;

        if (Board->CurrentColor == WHITE) {
            if (((BB_SQUARE(To) << 7) & ~BB_FILE_H) & Board->BB_Pieces[WHITE][PAWN]) {
                AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To + 7, To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
            }

            if (((BB_SQUARE(To) << 9) & ~BB_FILE_A) & Board->BB_Pieces[WHITE][PAWN]) {
                AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To + 9, To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
            }
        }
        else { // BLACK
            if (((BB_SQUARE(To) >> 7) & ~BB_FILE_A)& Board->BB_Pieces[BLACK][PAWN]) {
                AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To - 7, To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
            }

            if (((BB_SQUARE(To) >> 9) & ~BB_FILE_H)& Board->BB_Pieces[BLACK][PAWN]) {
                AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To - 9, To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
            }
        }
    }

    // Pawns pushed moves
    if (Board->CurrentColor == WHITE) {
        QuietMoves = PushedPawns(Board->BB_Pieces[WHITE][PAWN], WHITE, ~(Board->BB_WhitePieces | Board->BB_BlackPieces));

        while (QuietMoves) {
            To = LSB(QuietMoves);

            AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To + 8, To, MOVE_PAWN);

            QuietMoves &= QuietMoves - 1;
        }
    }
    else { // BLACK
        QuietMoves = PushedPawns(Board->BB_Pieces[BLACK][PAWN], BLACK, ~(Board->BB_WhitePieces | Board->BB_BlackPieces));

        while (QuietMoves) {
            To = LSB(QuietMoves);

            AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To - 8, To, MOVE_PAWN);

            QuietMoves &= QuietMoves - 1;
        }
    }

    // Pawns double pushed moves
    if (Board->CurrentColor == WHITE) {
        QuietMoves = PushedPawns2(Board->BB_Pieces[WHITE][PAWN], WHITE, ~(Board->BB_WhitePieces | Board->BB_BlackPieces));

        while (QuietMoves) {
            To = LSB(QuietMoves);

            AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To + 16, To, MOVE_PAWN_2);

            QuietMoves &= QuietMoves - 1;
        }
    }
    else { // BLACK
        QuietMoves = PushedPawns2(Board->BB_Pieces[BLACK][PAWN], BLACK, ~(Board->BB_WhitePieces | Board->BB_BlackPieces));

        while (QuietMoves) {
            To = LSB(QuietMoves);

            AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To - 16, To, MOVE_PAWN_2);

            QuietMoves &= QuietMoves - 1;
        }
    }

    // Knights captures and quiet moves
    Pieces = Board->BB_Pieces[Board->CurrentColor][KNIGHT];

    while (Pieces) {
        From = LSB(Pieces);

        Attacks = KnightAttacks(From);

        if (Board->CurrentColor == WHITE) {
            CaptureMoves = Attacks & Board->BB_BlackPieces;
        }
        else { // BLACK
            CaptureMoves = Attacks & Board->BB_WhitePieces;
        }

        while (CaptureMoves) {
            To = LSB(CaptureMoves);

            AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, From, To, MOVE_CAPTURE);

            CaptureMoves &= CaptureMoves - 1;
        }

        QuietMoves = Attacks & ~(Board->BB_WhitePieces | Board->BB_BlackPieces);

        while (QuietMoves) {
            To = LSB(QuietMoves);

            AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, From, To, MOVE_QUIET);

            QuietMoves &= QuietMoves - 1;
        }

        Pieces &= Pieces - 1;
    }

    // Bishops or Queens captures and quiet moves
    Pieces = Board->BB_Pieces[Board->CurrentColor][BISHOP];
    Pieces |= Board->BB_Pieces[Board->CurrentColor][QUEEN];

    while (Pieces) {
        From = LSB(Pieces);

        Attacks = BishopAttacks(From, (Board->BB_WhitePieces | Board->BB_BlackPieces));

        if (Board->CurrentColor == WHITE) {
            CaptureMoves = Attacks & Board->BB_BlackPieces;
        }
        else { // BLACK
            CaptureMoves = Attacks & Board->BB_WhitePieces;
        }

        while (CaptureMoves) {
            To = LSB(CaptureMoves);

            AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, From, To, MOVE_CAPTURE);

            CaptureMoves &= CaptureMoves - 1;
        }

        QuietMoves = Attacks & ~(Board->BB_WhitePieces | Board->BB_BlackPieces);

        while (QuietMoves) {
            To = LSB(QuietMoves);

            AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, From, To, MOVE_QUIET);

            QuietMoves &= QuietMoves - 1;
        }

        Pieces &= Pieces - 1;
    }

    // Rooks or Queens captures and quiet moves
    Pieces = Board->BB_Pieces[Board->CurrentColor][ROOK];
    Pieces |= Board->BB_Pieces[Board->CurrentColor][QUEEN];

    while (Pieces) {
        From = LSB(Pieces);

        Attacks = RookAttacks(From, (Board->BB_WhitePieces | Board->BB_BlackPieces));

        if (Board->CurrentColor == WHITE) {
            CaptureMoves = Attacks & Board->BB_BlackPieces;
        }
        else { // BLACK
            CaptureMoves = Attacks & Board->BB_WhitePieces;
        }

        while (CaptureMoves) {
            To = LSB(CaptureMoves);

            AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, From, To, MOVE_CAPTURE);

            CaptureMoves &= CaptureMoves - 1;
        }

        QuietMoves = Attacks & ~(Board->BB_WhitePieces | Board->BB_BlackPieces);

        while (QuietMoves) {
            To = LSB(QuietMoves);

            AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, From, To, MOVE_QUIET);

            QuietMoves &= QuietMoves - 1;
        }

        Pieces &= Pieces - 1;
    }

    // King captures and quiet moves
    From = LSB(Board->BB_Pieces[Board->CurrentColor][KING]);

    Attacks = KingAttacks(From);

    if (Board->CurrentColor == WHITE) {
        CaptureMoves = Attacks & Board->BB_BlackPieces;
    }
    else { // BLACK
        CaptureMoves = Attacks & Board->BB_WhitePieces;
    }

    while (CaptureMoves) {
        To = LSB(CaptureMoves);

        AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, From, To, MOVE_CAPTURE);

        CaptureMoves &= CaptureMoves - 1;
    }

    QuietMoves = Attacks & ~(Board->BB_WhitePieces | Board->BB_BlackPieces);

    while (QuietMoves) {
        To = LSB(QuietMoves);

        AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, From, To, MOVE_QUIET);

        QuietMoves &= QuietMoves - 1;
    }

    // King castle moves
    if (Board->CurrentColor == WHITE) {
        if (
            (Board->CastleFlags & CASTLE_WHITE_KING)
            && Board->Pieces[61] == EMPTY && Board->Pieces[62] == EMPTY // f1/g1
            && !IsSquareAttacked(Board, 60, Board->CurrentColor) && !IsSquareAttacked(Board, 61, Board->CurrentColor) && !IsSquareAttacked(Board, 62, Board->CurrentColor) // e1/f1/g1
        ) { // White O-O
            AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, 60, 62, MOVE_CASTLE_KING); // e1 -> g1
        }

        if (
            (Board->CastleFlags & CASTLE_WHITE_QUEEN)
            && Board->Pieces[59] == EMPTY && Board->Pieces[58] == EMPTY && Board->Pieces[57] == EMPTY // d1/c1/b1
            && !IsSquareAttacked(Board, 60, Board->CurrentColor) && !IsSquareAttacked(Board, 59, Board->CurrentColor) && !IsSquareAttacked(Board, 58, Board->CurrentColor) // e1/d1/c1
        ) { // White O-O-O
            AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, 60, 58, MOVE_CASTLE_QUEEN); // e1 -> c1
        }
    }
    else { // BLACK
        if (
            (Board->CastleFlags & CASTLE_BLACK_KING)
            && Board->Pieces[5] == EMPTY && Board->Pieces[6] == EMPTY // f8/g8
            && !IsSquareAttacked(Board, 4, Board->CurrentColor) && !IsSquareAttacked(Board, 5, Board->CurrentColor) && !IsSquareAttacked(Board, 6, Board->CurrentColor) // e8/f8/g8
        ) { // Black O-O
            AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, 4, 6, MOVE_CASTLE_KING); // e8 -> g8
        }

        if (
            (Board->CastleFlags & CASTLE_BLACK_QUEEN)
            && Board->Pieces[3] == EMPTY && Board->Pieces[2] == EMPTY && Board->Pieces[1] == EMPTY // d8/c8/b8
            && !IsSquareAttacked(Board, 4, Board->CurrentColor) && !IsSquareAttacked(Board, 3, Board->CurrentColor) && !IsSquareAttacked(Board, 2, Board->CurrentColor) // e8/d8/c8
        ) { // Black O-O-O
            AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, 4, 2, MOVE_CASTLE_QUEEN); // e8 -> c8
        }
    }
}

void GenerateCaptureMoves(const BoardItem* Board, int** CMH_Pointer, MoveItem* MoveList, int* GenMoveCount)
{
    U64 Pieces;
    U64 Attacks;

    U64 CaptureMoves;
    U64 QuietMoves;

    int From;
    int To;

    // Pawns captures
    if (Board->CurrentColor == WHITE) {
        Attacks = PawnAttacks(Board->BB_Pieces[WHITE][PAWN], WHITE) & Board->BB_BlackPieces;

        while (Attacks) {
            To = LSB(Attacks);

            if (((BB_SQUARE(To) << 7) & ~BB_FILE_H) & Board->BB_Pieces[WHITE][PAWN]) {
                AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To + 7, To, (MOVE_PAWN | MOVE_CAPTURE));
            }

            if (((BB_SQUARE(To) << 9) & ~BB_FILE_A) & Board->BB_Pieces[WHITE][PAWN]) {
                AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To + 9, To, (MOVE_PAWN | MOVE_CAPTURE));
            }

            Attacks &= Attacks - 1;
        }
    }
    else { // BLACK
        Attacks = PawnAttacks(Board->BB_Pieces[BLACK][PAWN], BLACK) & Board->BB_WhitePieces;

        while (Attacks) {
            To = LSB(Attacks);

            if (((BB_SQUARE(To) >> 7) & ~BB_FILE_A)& Board->BB_Pieces[BLACK][PAWN]) {
                AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To - 7, To, (MOVE_PAWN | MOVE_CAPTURE));
            }

            if (((BB_SQUARE(To) >> 9) & ~BB_FILE_H)& Board->BB_Pieces[BLACK][PAWN]) {
                AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To - 9, To, (MOVE_PAWN | MOVE_CAPTURE));
            }

            Attacks &= Attacks - 1;
        }
    }

    // Pawns captures (en passant)
    if (Board->PassantSquare != -1) {
        To = Board->PassantSquare;

        if (Board->CurrentColor == WHITE) {
            if (((BB_SQUARE(To) << 7) & ~BB_FILE_H) & Board->BB_Pieces[WHITE][PAWN]) {
                AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To + 7, To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
            }

            if (((BB_SQUARE(To) << 9) & ~BB_FILE_A) & Board->BB_Pieces[WHITE][PAWN]) {
                AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To + 9, To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
            }
        }
        else { // BLACK
            if (((BB_SQUARE(To) >> 7) & ~BB_FILE_A)& Board->BB_Pieces[BLACK][PAWN]) {
                AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To - 7, To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
            }

            if (((BB_SQUARE(To) >> 9) & ~BB_FILE_H)& Board->BB_Pieces[BLACK][PAWN]) {
                AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To - 9, To, (MOVE_PAWN_PASSANT | MOVE_CAPTURE));
            }
        }
    }

    // Pawns pushed moves (promote only)
    if (Board->CurrentColor == WHITE) {
        QuietMoves = PushedPawns(Board->BB_Pieces[WHITE][PAWN], WHITE, ~(Board->BB_WhitePieces | Board->BB_BlackPieces)) & BB_RANK_8;

        while (QuietMoves) {
            To = LSB(QuietMoves);

            AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To + 8, To, MOVE_PAWN);

            QuietMoves &= QuietMoves - 1;
        }
    }
    else { // BLACK
        QuietMoves = PushedPawns(Board->BB_Pieces[BLACK][PAWN], BLACK, ~(Board->BB_WhitePieces | Board->BB_BlackPieces)) & BB_RANK_1;

        while (QuietMoves) {
            To = LSB(QuietMoves);

            AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, To - 8, To, MOVE_PAWN);

            QuietMoves &= QuietMoves - 1;
        }
    }

    // Knights captures
    Pieces = Board->BB_Pieces[Board->CurrentColor][KNIGHT];

    while (Pieces) {
        From = LSB(Pieces);

        Attacks = KnightAttacks(From);

        if (Board->CurrentColor == WHITE) {
            CaptureMoves = Attacks & Board->BB_BlackPieces;
        }
        else { // BLACK
            CaptureMoves = Attacks & Board->BB_WhitePieces;
        }

        while (CaptureMoves) {
            To = LSB(CaptureMoves);

            AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, From, To, MOVE_CAPTURE);

            CaptureMoves &= CaptureMoves - 1;
        }

        Pieces &= Pieces - 1;
    }

    // Bishops or Queens captures
    Pieces = Board->BB_Pieces[Board->CurrentColor][BISHOP];
    Pieces |= Board->BB_Pieces[Board->CurrentColor][QUEEN];

    while (Pieces) {
        From = LSB(Pieces);

        Attacks = BishopAttacks(From, (Board->BB_WhitePieces | Board->BB_BlackPieces));

        if (Board->CurrentColor == WHITE) {
            CaptureMoves = Attacks & Board->BB_BlackPieces;
        }
        else { // BLACK
            CaptureMoves = Attacks & Board->BB_WhitePieces;
        }

        while (CaptureMoves) {
            To = LSB(CaptureMoves);

            AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, From, To, MOVE_CAPTURE);

            CaptureMoves &= CaptureMoves - 1;
        }

        Pieces &= Pieces - 1;
    }

    // Rooks or Queens captures
    Pieces = Board->BB_Pieces[Board->CurrentColor][ROOK];
    Pieces |= Board->BB_Pieces[Board->CurrentColor][QUEEN];

    while (Pieces) {
        From = LSB(Pieces);

        Attacks = RookAttacks(From, (Board->BB_WhitePieces | Board->BB_BlackPieces));

        if (Board->CurrentColor == WHITE) {
            CaptureMoves = Attacks & Board->BB_BlackPieces;
        }
        else { // BLACK
            CaptureMoves = Attacks & Board->BB_WhitePieces;
        }

        while (CaptureMoves) {
            To = LSB(CaptureMoves);

            AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, From, To, MOVE_CAPTURE);

            CaptureMoves &= CaptureMoves - 1;
        }

        Pieces &= Pieces - 1;
    }

    // King captures
    From = LSB(Board->BB_Pieces[Board->CurrentColor][KING]);

    Attacks = KingAttacks(From);

    if (Board->CurrentColor == WHITE) {
        CaptureMoves = Attacks & Board->BB_BlackPieces;
    }
    else { // BLACK
        CaptureMoves = Attacks & Board->BB_WhitePieces;
    }

    while (CaptureMoves) {
        To = LSB(CaptureMoves);

        AddMove(Board, CMH_Pointer, MoveList, GenMoveCount, From, To, MOVE_CAPTURE);

        CaptureMoves &= CaptureMoves - 1;
    }
}

void GenerateAllLegalMoves(BoardItem* Board, int** CMH_Pointer, MoveItem* LegalMoveList, int* LegalMoveCount)
{
    int GenMoveCount;
    MoveItem MoveList[MAX_GEN_MOVES];

    GenMoveCount = 0;
    GenerateAllMoves(Board, CMH_Pointer, MoveList, &GenMoveCount);

    for (int MoveNumber = 0; MoveNumber < GenMoveCount; ++MoveNumber) {
        MakeMove(Board, MoveList[MoveNumber]);

        if (!IsInCheck(Board, CHANGE_COLOR(Board->CurrentColor))) { // Legal move
            LegalMoveList[(*LegalMoveCount)++] = MoveList[MoveNumber];
        }

        UnmakeMove(Board);
    }
}