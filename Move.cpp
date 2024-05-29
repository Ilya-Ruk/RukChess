// Move.cpp

#include "stdafx.h"

#include "Move.h"

#include "BitBoard.h"
#include "Board.h"
#include "Def.h"
#include "Evaluate.h"
#include "Hash.h"
#include "Types.h"

void MakeMove(BoardItem* Board, const MoveItem Move)
{
    HistoryItem* Info = &Board->MoveTable[Board->HalfMoveNumber++];

    int From = MOVE_FROM(Move.Move);
    int To = MOVE_TO(Move.Move);

    Info->Type = Move.Type;

    Info->From = From;
    Info->PieceFrom = PIECE(Board->Pieces[From]);

    Info->To = To;
    Info->PieceTo = PIECE(Board->Pieces[To]);

    Info->PromotePiece = MOVE_PROMOTE_PIECE(Move.Move);

    Info->PassantSquare = Board->PassantSquare;

    Info->CastleFlags = Board->CastleFlags;
    Info->FiftyMove = Board->FiftyMove;

    Info->Hash = Board->Hash;

#ifdef DEBUG_MOVE
    Info->BB_WhitePieces = Board->BB_WhitePieces;
    Info->BB_BlackPieces = Board->BB_BlackPieces;

    for (int Color = 0; Color < 2; ++Color) {
        for (int Piece = 0; Piece < 6; ++Piece) {
            Info->BB_Pieces[Color][Piece] = Board->BB_Pieces[Color][Piece];
        }
    }
#endif // DEBUG_MOVE

    if (Info->PassantSquare != -1) {
        Board->PassantSquare = -1;

        Board->Hash ^= PassantHash[Info->PassantSquare];
    }

    if (Board->CurrentColor == WHITE) {
        if (Move.Type & MOVE_PAWN_2) {
            Board->PassantSquare = From - 8;

            Board->Hash ^= PassantHash[Board->PassantSquare];
        }

        if (Move.Type & MOVE_CASTLE_KING) { // White O-O
            Board->Pieces[63] = EMPTY;

            Board->BB_WhitePieces &= ~BB_SQUARE(63);
            Board->BB_Pieces[WHITE][ROOK] &= ~BB_SQUARE(63);

            Board->Hash ^= PieceHash[WHITE][ROOK][63];

            Board->Pieces[61] = PIECE_AND_COLOR(ROOK, WHITE);

            Board->BB_WhitePieces |= BB_SQUARE(61);
            Board->BB_Pieces[WHITE][ROOK] |= BB_SQUARE(61);

            Board->Hash ^= PieceHash[WHITE][ROOK][61];
        }

        if (Move.Type & MOVE_CASTLE_QUEEN) { // White O-O-O
            Board->Pieces[56] = EMPTY;

            Board->BB_WhitePieces &= ~BB_SQUARE(56);
            Board->BB_Pieces[WHITE][ROOK] &= ~BB_SQUARE(56);

            Board->Hash ^= PieceHash[WHITE][ROOK][56];

            Board->Pieces[59] = PIECE_AND_COLOR(ROOK, WHITE);

            Board->BB_WhitePieces |= BB_SQUARE(59);
            Board->BB_Pieces[WHITE][ROOK] |= BB_SQUARE(59);

            Board->Hash ^= PieceHash[WHITE][ROOK][59];
        }

        Board->CastleFlags &= CastleMask[From] & CastleMask[To];

        if (Move.Type & MOVE_PAWN_PASSANT) {
            Info->EatPawnSquare = To + 8;

            Board->Pieces[Info->EatPawnSquare] = EMPTY;

            Board->BB_BlackPieces &= ~BB_SQUARE(Info->EatPawnSquare);
            Board->BB_Pieces[BLACK][PAWN] &= ~BB_SQUARE(Info->EatPawnSquare);

            Board->Hash ^= PieceHash[BLACK][PAWN][Info->EatPawnSquare];
        }
        else if (Move.Type & MOVE_CAPTURE) {
            Board->BB_BlackPieces &= ~BB_SQUARE(To);
            Board->BB_Pieces[BLACK][Info->PieceTo] &= ~BB_SQUARE(To);

            Board->Hash ^= PieceHash[BLACK][Info->PieceTo][To];
        }

        if (Move.Type & MOVE_PAWN_PROMOTE) {
            Board->Pieces[To] = PIECE_AND_COLOR(Info->PromotePiece, WHITE);

            Board->BB_WhitePieces |= BB_SQUARE(To);
            Board->BB_Pieces[WHITE][Info->PromotePiece] |= BB_SQUARE(To);

            Board->Hash ^= PieceHash[WHITE][Info->PromotePiece][To];
        }
        else {
            Board->Pieces[To] = Board->Pieces[From];

            Board->BB_WhitePieces |= BB_SQUARE(To);
            Board->BB_Pieces[WHITE][Info->PieceFrom] |= BB_SQUARE(To);

            Board->Hash ^= PieceHash[WHITE][Info->PieceFrom][To];
        }

        Board->Pieces[From] = EMPTY;

        Board->BB_WhitePieces &= ~BB_SQUARE(From);
        Board->BB_Pieces[WHITE][Info->PieceFrom] &= ~BB_SQUARE(From);

        Board->Hash ^= PieceHash[WHITE][Info->PieceFrom][From];
    }
    else { // BLACK
        if (Move.Type & MOVE_PAWN_2) {
            Board->PassantSquare = From + 8;

            Board->Hash ^= PassantHash[Board->PassantSquare];
        }

        if (Move.Type & MOVE_CASTLE_KING) { // Black O-O
            Board->Pieces[7] = EMPTY;

            Board->BB_BlackPieces &= ~BB_SQUARE(7);
            Board->BB_Pieces[BLACK][ROOK] &= ~BB_SQUARE(7);

            Board->Hash ^= PieceHash[BLACK][ROOK][7];

            Board->Pieces[5] = PIECE_AND_COLOR(ROOK, BLACK);

            Board->BB_BlackPieces |= BB_SQUARE(5);
            Board->BB_Pieces[BLACK][ROOK] |= BB_SQUARE(5);

            Board->Hash ^= PieceHash[BLACK][ROOK][5];
        }

        if (Move.Type & MOVE_CASTLE_QUEEN) { // Black O-O-O
            Board->Pieces[0] = EMPTY;

            Board->BB_BlackPieces &= ~BB_SQUARE(0);
            Board->BB_Pieces[BLACK][ROOK] &= ~BB_SQUARE(0);

            Board->Hash ^= PieceHash[BLACK][ROOK][0];

            Board->Pieces[3] = PIECE_AND_COLOR(ROOK, BLACK);

            Board->BB_BlackPieces |= BB_SQUARE(3);
            Board->BB_Pieces[BLACK][ROOK] |= BB_SQUARE(3);

            Board->Hash ^= PieceHash[BLACK][ROOK][3];
        }

        Board->CastleFlags &= CastleMask[From] & CastleMask[To];

        if (Move.Type & MOVE_PAWN_PASSANT) {
            Info->EatPawnSquare = To - 8;

            Board->Pieces[Info->EatPawnSquare] = EMPTY;

            Board->BB_WhitePieces &= ~BB_SQUARE(Info->EatPawnSquare);
            Board->BB_Pieces[WHITE][PAWN] &= ~BB_SQUARE(Info->EatPawnSquare);

            Board->Hash ^= PieceHash[WHITE][PAWN][Info->EatPawnSquare];
        }
        else if (Move.Type & MOVE_CAPTURE) {
            Board->BB_WhitePieces &= ~BB_SQUARE(To);
            Board->BB_Pieces[WHITE][Info->PieceTo] &= ~BB_SQUARE(To);

            Board->Hash ^= PieceHash[WHITE][Info->PieceTo][To];
        }

        if (Move.Type & MOVE_PAWN_PROMOTE) {
            Board->Pieces[To] = PIECE_AND_COLOR(Info->PromotePiece, BLACK);

            Board->BB_BlackPieces |= BB_SQUARE(To);
            Board->BB_Pieces[BLACK][Info->PromotePiece] |= BB_SQUARE(To);

            Board->Hash ^= PieceHash[BLACK][Info->PromotePiece][To];
        }
        else {
            Board->Pieces[To] = Board->Pieces[From];

            Board->BB_BlackPieces |= BB_SQUARE(To);
            Board->BB_Pieces[BLACK][Info->PieceFrom] |= BB_SQUARE(To);

            Board->Hash ^= PieceHash[BLACK][Info->PieceFrom][To];
        }

        Board->Pieces[From] = EMPTY;

        Board->BB_BlackPieces &= ~BB_SQUARE(From);
        Board->BB_Pieces[BLACK][Info->PieceFrom] &= ~BB_SQUARE(From);

        Board->Hash ^= PieceHash[BLACK][Info->PieceFrom][From];
    }

    if (Move.Type & (MOVE_CAPTURE | MOVE_PAWN | MOVE_PAWN_2)) {
        Board->FiftyMove = 0;
    }
    else {
        ++Board->FiftyMove;
    }

    Board->CurrentColor ^= 1;

    Board->Hash ^= ColorHash;

#ifdef DEBUG_HASH
    U64 OldBoardHash = Board->Hash;

    InitHash(Board);

    if (Board->Hash != OldBoardHash) {
        printf("-- Board hash error! BoardHash = 0x%016llx OldBoardHash = 0x%016llx\n", Board->Hash, OldBoardHash);
    }
#endif // DEBUG_HASH
}

void UnmakeMove(BoardItem* Board)
{
    HistoryItem* Info = &Board->MoveTable[--Board->HalfMoveNumber];

    Board->CurrentColor ^= 1;

    Board->Pieces[Info->From] = PIECE_AND_COLOR(Info->PieceFrom, Board->CurrentColor);

    if (Board->CurrentColor == WHITE) {
        Board->BB_WhitePieces |= BB_SQUARE(Info->From);
        Board->BB_Pieces[WHITE][Info->PieceFrom] |= BB_SQUARE(Info->From);

        if (Info->Type & MOVE_CASTLE_KING) { // White O-O
            Board->Pieces[61] = EMPTY;

            Board->BB_WhitePieces &= ~BB_SQUARE(61);
            Board->BB_Pieces[WHITE][ROOK] &= ~BB_SQUARE(61);

            Board->Pieces[63] = PIECE_AND_COLOR(ROOK, WHITE);

            Board->BB_WhitePieces |= BB_SQUARE(63);
            Board->BB_Pieces[WHITE][ROOK] |= BB_SQUARE(63);
        }

        if (Info->Type & MOVE_CASTLE_QUEEN) { // White O-O-O
            Board->Pieces[59] = EMPTY;

            Board->BB_WhitePieces &= ~BB_SQUARE(59);
            Board->BB_Pieces[WHITE][ROOK] &= ~BB_SQUARE(59);

            Board->Pieces[56] = PIECE_AND_COLOR(ROOK, WHITE);

            Board->BB_WhitePieces |= BB_SQUARE(56);
            Board->BB_Pieces[WHITE][ROOK] |= BB_SQUARE(56);
        }

        if (Info->Type & MOVE_PAWN_PASSANT) {
            Board->Pieces[Info->To] = EMPTY;

            Board->BB_WhitePieces &= ~BB_SQUARE(Info->To);
            Board->BB_Pieces[WHITE][PAWN] &= ~BB_SQUARE(Info->To);

            Board->Pieces[Info->EatPawnSquare] = PIECE_AND_COLOR(PAWN, BLACK);

            Board->BB_BlackPieces |= BB_SQUARE(Info->EatPawnSquare);
            Board->BB_Pieces[BLACK][PAWN] |= BB_SQUARE(Info->EatPawnSquare);
        }
        else if (Info->Type & MOVE_CAPTURE) {
            Board->BB_WhitePieces &= ~BB_SQUARE(Info->To);

            if (Info->Type & MOVE_PAWN_PROMOTE) {
                Board->BB_Pieces[WHITE][Info->PromotePiece] &= ~BB_SQUARE(Info->To);
            }
            else {
                Board->BB_Pieces[WHITE][Info->PieceFrom] &= ~BB_SQUARE(Info->To);
            }

            Board->Pieces[Info->To] = PIECE_AND_COLOR(Info->PieceTo, BLACK);

            Board->BB_BlackPieces |= BB_SQUARE(Info->To);
            Board->BB_Pieces[BLACK][Info->PieceTo] |= BB_SQUARE(Info->To);
        }
        else {
            Board->BB_WhitePieces &= ~BB_SQUARE(Info->To);

            if (Info->Type & MOVE_PAWN_PROMOTE) {
                Board->BB_Pieces[WHITE][Info->PromotePiece] &= ~BB_SQUARE(Info->To);
            }
            else {
                Board->BB_Pieces[WHITE][Info->PieceFrom] &= ~BB_SQUARE(Info->To);
            }

            Board->Pieces[Info->To] = EMPTY;
        }
    }
    else { // BLACK
        Board->BB_BlackPieces |= BB_SQUARE(Info->From);
        Board->BB_Pieces[BLACK][Info->PieceFrom] |= BB_SQUARE(Info->From);

        if (Info->Type & MOVE_CASTLE_KING) { // Black O-O
            Board->Pieces[5] = EMPTY;

            Board->BB_BlackPieces &= ~BB_SQUARE(5);
            Board->BB_Pieces[BLACK][ROOK] &= ~BB_SQUARE(5);

            Board->Pieces[7] = PIECE_AND_COLOR(ROOK, BLACK);

            Board->BB_BlackPieces |= BB_SQUARE(7);
            Board->BB_Pieces[BLACK][ROOK] |= BB_SQUARE(7);
        }

        if (Info->Type & MOVE_CASTLE_QUEEN) { // Black O-O-O
            Board->Pieces[3] = EMPTY;

            Board->BB_BlackPieces &= ~BB_SQUARE(3);
            Board->BB_Pieces[BLACK][ROOK] &= ~BB_SQUARE(3);

            Board->Pieces[0] = PIECE_AND_COLOR(ROOK, BLACK);

            Board->BB_BlackPieces |= BB_SQUARE(0);
            Board->BB_Pieces[BLACK][ROOK] |= BB_SQUARE(0);
        }

        if (Info->Type & MOVE_PAWN_PASSANT) {
            Board->Pieces[Info->To] = EMPTY;

            Board->BB_BlackPieces &= ~BB_SQUARE(Info->To);
            Board->BB_Pieces[BLACK][PAWN] &= ~BB_SQUARE(Info->To);

            Board->Pieces[Info->EatPawnSquare] = PIECE_AND_COLOR(PAWN, WHITE);

            Board->BB_WhitePieces |= BB_SQUARE(Info->EatPawnSquare);
            Board->BB_Pieces[WHITE][PAWN] |= BB_SQUARE(Info->EatPawnSquare);
        }
        else if (Info->Type & MOVE_CAPTURE) {
            Board->BB_BlackPieces &= ~BB_SQUARE(Info->To);

            if (Info->Type & MOVE_PAWN_PROMOTE) {
                Board->BB_Pieces[BLACK][Info->PromotePiece] &= ~BB_SQUARE(Info->To);
            }
            else {
                Board->BB_Pieces[BLACK][Info->PieceFrom] &= ~BB_SQUARE(Info->To);
            }

            Board->Pieces[Info->To] = PIECE_AND_COLOR(Info->PieceTo, WHITE);

            Board->BB_WhitePieces |= BB_SQUARE(Info->To);
            Board->BB_Pieces[WHITE][Info->PieceTo] |= BB_SQUARE(Info->To);
        }
        else {
            Board->BB_BlackPieces &= ~BB_SQUARE(Info->To);

            if (Info->Type & MOVE_PAWN_PROMOTE) {
                Board->BB_Pieces[BLACK][Info->PromotePiece] &= ~BB_SQUARE(Info->To);
            }
            else {
                Board->BB_Pieces[BLACK][Info->PieceFrom] &= ~BB_SQUARE(Info->To);
            }

            Board->Pieces[Info->To] = EMPTY;
        }
    }

    Board->PassantSquare = Info->PassantSquare;

    Board->CastleFlags = Info->CastleFlags;
    Board->FiftyMove = Info->FiftyMove;

    Board->Hash = Info->Hash;

#ifdef DEBUG_MOVE
    if (Board->BB_WhitePieces != Info->BB_WhitePieces) {
        printf("-- BB_WhitePieces error! From = %d To = %d Move type = %d\n", Info->From, Info->To, Info->Type);
    }

    if (Board->BB_BlackPieces != Info->BB_BlackPieces) {
        printf("-- BB_BlackPieces error! From = %d To = %d Move type = %d\n", Info->From, Info->To, Info->Type);
    }

    for (int Color = 0; Color < 2; ++Color) {
        for (int Piece = 0; Piece < 6; ++Piece) {
            if (Board->BB_Pieces[Color][Piece] != Info->BB_Pieces[Color][Piece]) {
                printf("-- BB_Pieces error! Color = %d Piece = %d From = %d To = %d Move type = %d\n", Color, Piece, Info->From, Info->To, Info->Type);
            }
        }
    }
#endif // DEBUG_MOVE
}

#ifdef NULL_MOVE_PRUNING

void MakeNullMove(BoardItem* Board)
{
    HistoryItem* Info = &Board->MoveTable[Board->HalfMoveNumber++];

    Info->Type = MOVE_NULL;

    Info->PassantSquare = Board->PassantSquare;

    Info->FiftyMove = Board->FiftyMove;

    Info->Hash = Board->Hash;

    if (Info->PassantSquare != -1) {
        Board->PassantSquare = -1;

        Board->Hash ^= PassantHash[Info->PassantSquare];
    }

    ++Board->FiftyMove;

    Board->CurrentColor ^= 1;

    Board->Hash ^= ColorHash;

#ifdef DEBUG_HASH
    U64 OldBoardHash = Board->Hash;

    InitHash(Board);

    if (Board->Hash != OldBoardHash) {
        printf("-- Board hash error! BoardHash = 0x%016llx OldBoardHash = 0x%016llx\n", Board->Hash, OldBoardHash);
    }
#endif // DEBUG_HASH
}

void UnmakeNullMove(BoardItem* Board)
{
    HistoryItem* Info = &Board->MoveTable[--Board->HalfMoveNumber];

    Board->CurrentColor ^= 1;

    Board->PassantSquare = Info->PassantSquare;

    Board->FiftyMove = Info->FiftyMove;

    Board->Hash = Info->Hash;
}

#endif // NULL_MOVE_PRUNING