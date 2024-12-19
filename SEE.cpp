// SEE.cpp

#include "stdafx.h"

#include "SEE.h"

#include "BitBoard.h"
#include "Board.h"
#include "Def.h"
#include "Types.h"
#include "Utils.h"

#if defined(PROBCUT) || defined(BAD_CAPTURE_LAST) || defined(SEE_CAPTURE_MOVE_PRUNING) || defined(SEE_QUIET_MOVE_PRUNING) || defined(QUIESCENCE_SEE_MOVE_PRUNING)

const int PiecesScoreSEE[6] = { 100, 300, 300, 500, 900, INF }; // PNBRQK

U64 AttackTo(const BoardItem* Board, const int Square, const U64 Occupied)
{
    U64 Attackers = 0ULL;

    // Pawns
    Attackers |= PawnAttacks(BB_SQUARE(Square), WHITE) & Board->BB_Pieces[BLACK][PAWN];
    Attackers |= PawnAttacks(BB_SQUARE(Square), BLACK) & Board->BB_Pieces[WHITE][PAWN];

    // Knights
    Attackers |= KnightAttacks(Square) & (Board->BB_Pieces[WHITE][KNIGHT] | Board->BB_Pieces[BLACK][KNIGHT]);

    // Bishops or Queens
    Attackers |= BishopAttacks(Square, Occupied) & (Board->BB_Pieces[WHITE][BISHOP] | Board->BB_Pieces[WHITE][QUEEN] | Board->BB_Pieces[BLACK][BISHOP] | Board->BB_Pieces[BLACK][QUEEN]);

    // Rooks or Queens
    Attackers |= RookAttacks(Square, Occupied) & (Board->BB_Pieces[WHITE][ROOK] | Board->BB_Pieces[WHITE][QUEEN] | Board->BB_Pieces[BLACK][ROOK] | Board->BB_Pieces[BLACK][QUEEN]);

    // Kings
    Attackers |= KingAttacks(Square) & (Board->BB_Pieces[WHITE][KING] | Board->BB_Pieces[BLACK][KING]);

    return Attackers;
}

int SEE(const BoardItem* Board, const int MoveType, const int Move)
{
    int Gain[32];

    int From = MOVE_FROM(Move);
    int To = MOVE_TO(Move);

    int PieceType;

    U64 BB_From = BB_SQUARE(From);

    U64 Occupied = Board->BB_WhitePieces | Board->BB_BlackPieces;

    U64 Attackers;
    U64 CurrentAttackers;

    U64 BishopsOrQueens = Board->BB_Pieces[WHITE][BISHOP] | Board->BB_Pieces[WHITE][QUEEN] | Board->BB_Pieces[BLACK][BISHOP] | Board->BB_Pieces[BLACK][QUEEN];
    U64 RooksOrQueens = Board->BB_Pieces[WHITE][ROOK] | Board->BB_Pieces[WHITE][QUEEN] | Board->BB_Pieces[BLACK][ROOK] | Board->BB_Pieces[BLACK][QUEEN];

    int Color = Board->CurrentColor;

    int Depth = 1;

#ifdef DEBUG_SEE
    printf("-- SEE: Move = %s%s\n", BoardName[From], BoardName[To]);
#endif // DEBUG_SEE

    if (MoveType & MOVE_PAWN_PASSANT) {
        Gain[0] = PiecesScoreSEE[PAWN];

        if (Color == WHITE) {
            Occupied &= ~BB_SQUARE(To + 8);
        }
        else { // BLACK
            Occupied &= ~BB_SQUARE(To - 8);
        }
    }
    else if (MoveType & MOVE_CAPTURE) {
        Gain[0] = PiecesScoreSEE[PIECE_TYPE(Board->Pieces[To])];
    }
    else {
        Gain[0] = 0;
    }

    if (MoveType & MOVE_PAWN_PROMOTE) {
        PieceType = MOVE_PROMOTE_PIECE_TYPE(Move);

        Gain[0] += PiecesScoreSEE[PieceType] - PiecesScoreSEE[PAWN];
    }
    else {
        PieceType = PIECE_TYPE(Board->Pieces[From]);
    }

    Occupied &= ~BB_From;

    Attackers = AttackTo(Board, To, Occupied);

    while (TRUE) {
        Color ^= 1;

        if (Color == WHITE) {
            CurrentAttackers = Attackers & Board->BB_WhitePieces;
        }
        else { // BLACK
            CurrentAttackers = Attackers & Board->BB_BlackPieces;
        }

        if (!CurrentAttackers) {
            break; // while
        }

        Gain[Depth] = -Gain[Depth - 1] + PiecesScoreSEE[PieceType];

        ++Depth;

        if (PieceType == KING) {
            break; // while
        }

        for (PieceType = 0; PieceType < 6; ++PieceType) { // PNBRQK
            if (CurrentAttackers & Board->BB_Pieces[Color][PieceType]) {
#ifdef DEBUG_SEE
                printf("-- SEE: From = %s\n", BoardName[LSB(CurrentAttackers & Board->BB_Pieces[Color][PieceType])]);
#endif // DEBUG_SEE

                BB_From = BB_SQUARE(LSB(CurrentAttackers & Board->BB_Pieces[Color][PieceType]));

                break; // for
            }
        }

        Occupied &= ~BB_From;

        // Add X-ray attacks: bishops or queens
        Attackers |= BishopAttacks(To, Occupied) & BishopsOrQueens;

        // Add X-ray attacks: rooks or queens
        Attackers |= RookAttacks(To, Occupied) & RooksOrQueens;

        Attackers &= Occupied;

        if (PieceType == PAWN && (RANK(To) == 0 || RANK(To) == 7)) { // Pawn promote
            PieceType = QUEEN;

            Gain[Depth] += PiecesScoreSEE[QUEEN] - PiecesScoreSEE[PAWN];
        }
    } // while

    while (--Depth) {
        Gain[Depth - 1] = MIN(-Gain[Depth], Gain[Depth - 1]);

#ifdef DEBUG_SEE
        printf("-- SEE: Gain[%d] = %d\n", Depth - 1, Gain[Depth - 1]);
#endif // DEBUG_SEE
    }

    return Gain[0];
}

#endif // PROBCUT || BAD_CAPTURE_LAST || SEE_CAPTURE_MOVE_PRUNING || SEE_QUIET_MOVE_PRUNING || QUIESCENCE_SEE_MOVE_PRUNING