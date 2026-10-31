// Move.cpp

#include "stdafx.h"

#include "Move.h"

#include "BitBoard.h"
#include "Board.h"
#include "Def.h"
#include "Hash.h"
#include "NNUE2.h"
#include "Types.h"

void MakeMove(BoardItem* Board, const MoveItem Move)
{
    HistoryItem* Info = &Board->MoveTable[Board->HalfMoveNumber++];

    int From = MOVE_FROM(Move.Move);
    int To = MOVE_TO(Move.Move);

    assert(From >= SQ_A8 && From <= SQ_H1);
    assert(To >= SQ_A8 && To <= SQ_H1);

    Info->Type = Move.Type;

    Info->From = From;
    Info->PieceTypeFrom = PIECE_TYPE(Board->Pieces[From]);

    assert(Info->PieceTypeFrom >= PAWN && Info->PieceTypeFrom <= KING);

    Info->To = To;
    Info->PieceTypeTo = PIECE_TYPE(Board->Pieces[To]);

    assert((Info->PieceTypeTo >= PAWN && Info->PieceTypeTo <= QUEEN) || Info->PieceTypeTo == PIECE_TYPE(NO_PIECE));

    Info->PromotePieceType = MOVE_PROMOTE_PIECE_TYPE(Move.Move);

    assert((Info->PromotePieceType >= KNIGHT && Info->PromotePieceType <= QUEEN) || Info->PromotePieceType == 0);

    Info->EnPassantSquare = Board->EnPassantSquare;

    assert((Info->EnPassantSquare >= SQ_A6 && Info->EnPassantSquare <= SQ_H6) || (Info->EnPassantSquare >= SQ_A3 && Info->EnPassantSquare <= SQ_H3) || Info->EnPassantSquare == -1);

    Info->CastleFlags = Board->CastleFlags;
    Info->FiftyMove = Board->FiftyMove;

    Info->Hash = Board->Hash;

#ifdef DEBUG_MOVE
    Info->BB_WhitePieces = Board->BB_WhitePieces;
    Info->BB_BlackPieces = Board->BB_BlackPieces;

    for (int Color = 0; Color < 2; ++Color) { // White/Black
        for (int Piece = 0; Piece < 6; ++Piece) { // PNBRQK
            Info->BB_Pieces[Color][Piece] = Board->BB_Pieces[Color][Piece];
        }
    }
#endif // DEBUG_MOVE

    if (Info->EnPassantSquare != -1) {
        Board->EnPassantSquare = -1;

        Board->Hash ^= PassantHash[Info->EnPassantSquare];
    }

    if (Board->CurrentColor == WHITE) {
        if (Move.Type & MOVE_PAWN_2) {
            Board->EnPassantSquare = From - 8;

            Board->Hash ^= PassantHash[Board->EnPassantSquare];
        }

        if (Move.Type & MOVE_CASTLE_KING) { // White O-O
            Board->Pieces[SQ_H1] = NO_PIECE;

            Board->BB_WhitePieces &= ~BB_SQUARE(SQ_H1);
            Board->BB_Pieces[WHITE][ROOK] &= ~BB_SQUARE(SQ_H1);

            Board->Hash ^= PieceHash[WHITE][ROOK][SQ_H1];

            AccumulatorSub(Board, SQ_H1, PIECE_CREATE(ROOK, WHITE));

            Board->Pieces[SQ_F1] = PIECE_CREATE(ROOK, WHITE);

            Board->BB_WhitePieces |= BB_SQUARE(SQ_F1);
            Board->BB_Pieces[WHITE][ROOK] |= BB_SQUARE(SQ_F1);

            Board->Hash ^= PieceHash[WHITE][ROOK][SQ_F1];

            AccumulatorAdd(Board, SQ_F1, PIECE_CREATE(ROOK, WHITE));
        }

        if (Move.Type & MOVE_CASTLE_QUEEN) { // White O-O-O
            Board->Pieces[SQ_A1] = NO_PIECE;

            Board->BB_WhitePieces &= ~BB_SQUARE(SQ_A1);
            Board->BB_Pieces[WHITE][ROOK] &= ~BB_SQUARE(SQ_A1);

            Board->Hash ^= PieceHash[WHITE][ROOK][SQ_A1];

            AccumulatorSub(Board, SQ_A1, PIECE_CREATE(ROOK, WHITE));

            Board->Pieces[SQ_D1] = PIECE_CREATE(ROOK, WHITE);

            Board->BB_WhitePieces |= BB_SQUARE(SQ_D1);
            Board->BB_Pieces[WHITE][ROOK] |= BB_SQUARE(SQ_D1);

            Board->Hash ^= PieceHash[WHITE][ROOK][SQ_D1];

            AccumulatorAdd(Board, SQ_D1, PIECE_CREATE(ROOK, WHITE));
        }

        Board->CastleFlags &= CastleMask[From] & CastleMask[To];

        if (Move.Type & MOVE_PAWN_EN_PASSANT) {
            Info->CapturePawnSquare = To + 8;

            assert(Info->CapturePawnSquare >= SQ_A5 && Info->CapturePawnSquare <= SQ_H5);

            Board->Pieces[Info->CapturePawnSquare] = NO_PIECE;

            Board->BB_BlackPieces &= ~BB_SQUARE(Info->CapturePawnSquare);
            Board->BB_Pieces[BLACK][PAWN] &= ~BB_SQUARE(Info->CapturePawnSquare);

            Board->Hash ^= PieceHash[BLACK][PAWN][Info->CapturePawnSquare];

            AccumulatorSub(Board, Info->CapturePawnSquare, PIECE_CREATE(PAWN, BLACK));
        }
        else if (Move.Type & MOVE_CAPTURE) {
            Board->Pieces[To] = NO_PIECE;

            Board->BB_BlackPieces &= ~BB_SQUARE(To);
            Board->BB_Pieces[BLACK][Info->PieceTypeTo] &= ~BB_SQUARE(To);

            Board->Hash ^= PieceHash[BLACK][Info->PieceTypeTo][To];

            AccumulatorSub(Board, To, PIECE_CREATE(Info->PieceTypeTo, BLACK));
        }

        if (Move.Type & MOVE_PAWN_PROMOTE) {
            Board->Pieces[To] = PIECE_CREATE(Info->PromotePieceType, WHITE);

            Board->BB_WhitePieces |= BB_SQUARE(To);
            Board->BB_Pieces[WHITE][Info->PromotePieceType] |= BB_SQUARE(To);

            Board->Hash ^= PieceHash[WHITE][Info->PromotePieceType][To];

            AccumulatorAdd(Board, To, PIECE_CREATE(Info->PromotePieceType, WHITE));
        }
        else {
            Board->Pieces[To] = Board->Pieces[From];

            Board->BB_WhitePieces |= BB_SQUARE(To);
            Board->BB_Pieces[WHITE][Info->PieceTypeFrom] |= BB_SQUARE(To);

            Board->Hash ^= PieceHash[WHITE][Info->PieceTypeFrom][To];

            AccumulatorAdd(Board, To, PIECE_CREATE(Info->PieceTypeFrom, WHITE));
        }

        Board->Pieces[From] = NO_PIECE;

        Board->BB_WhitePieces &= ~BB_SQUARE(From);
        Board->BB_Pieces[WHITE][Info->PieceTypeFrom] &= ~BB_SQUARE(From);

        Board->Hash ^= PieceHash[WHITE][Info->PieceTypeFrom][From];

        AccumulatorSub(Board, From, PIECE_CREATE(Info->PieceTypeFrom, WHITE));
    }
    else { // BLACK
        if (Move.Type & MOVE_PAWN_2) {
            Board->EnPassantSquare = From + 8;

            Board->Hash ^= PassantHash[Board->EnPassantSquare];
        }

        if (Move.Type & MOVE_CASTLE_KING) { // Black O-O
            Board->Pieces[SQ_H8] = NO_PIECE;

            Board->BB_BlackPieces &= ~BB_SQUARE(SQ_H8);
            Board->BB_Pieces[BLACK][ROOK] &= ~BB_SQUARE(SQ_H8);

            Board->Hash ^= PieceHash[BLACK][ROOK][SQ_H8];

            AccumulatorSub(Board, SQ_H8, PIECE_CREATE(ROOK, BLACK));

            Board->Pieces[SQ_F8] = PIECE_CREATE(ROOK, BLACK);

            Board->BB_BlackPieces |= BB_SQUARE(SQ_F8);
            Board->BB_Pieces[BLACK][ROOK] |= BB_SQUARE(SQ_F8);

            Board->Hash ^= PieceHash[BLACK][ROOK][SQ_F8];

            AccumulatorAdd(Board, SQ_F8, PIECE_CREATE(ROOK, BLACK));
        }

        if (Move.Type & MOVE_CASTLE_QUEEN) { // Black O-O-O
            Board->Pieces[SQ_A8] = NO_PIECE;

            Board->BB_BlackPieces &= ~BB_SQUARE(SQ_A8);
            Board->BB_Pieces[BLACK][ROOK] &= ~BB_SQUARE(SQ_A8);

            Board->Hash ^= PieceHash[BLACK][ROOK][SQ_A8];

            AccumulatorSub(Board, SQ_A8, PIECE_CREATE(ROOK, BLACK));

            Board->Pieces[SQ_D8] = PIECE_CREATE(ROOK, BLACK);

            Board->BB_BlackPieces |= BB_SQUARE(SQ_D8);
            Board->BB_Pieces[BLACK][ROOK] |= BB_SQUARE(SQ_D8);

            Board->Hash ^= PieceHash[BLACK][ROOK][SQ_D8];

            AccumulatorAdd(Board, SQ_D8, PIECE_CREATE(ROOK, BLACK));
        }

        Board->CastleFlags &= CastleMask[From] & CastleMask[To];

        if (Move.Type & MOVE_PAWN_EN_PASSANT) {
            Info->CapturePawnSquare = To - 8;

            assert(Info->CapturePawnSquare >= SQ_A4 && Info->CapturePawnSquare <= SQ_H4);

            Board->Pieces[Info->CapturePawnSquare] = NO_PIECE;

            Board->BB_WhitePieces &= ~BB_SQUARE(Info->CapturePawnSquare);
            Board->BB_Pieces[WHITE][PAWN] &= ~BB_SQUARE(Info->CapturePawnSquare);

            Board->Hash ^= PieceHash[WHITE][PAWN][Info->CapturePawnSquare];

            AccumulatorSub(Board, Info->CapturePawnSquare, PIECE_CREATE(PAWN, WHITE));
        }
        else if (Move.Type & MOVE_CAPTURE) {
            Board->Pieces[To] = NO_PIECE;

            Board->BB_WhitePieces &= ~BB_SQUARE(To);
            Board->BB_Pieces[WHITE][Info->PieceTypeTo] &= ~BB_SQUARE(To);

            Board->Hash ^= PieceHash[WHITE][Info->PieceTypeTo][To];

            AccumulatorSub(Board, To, PIECE_CREATE(Info->PieceTypeTo, WHITE));
        }

        if (Move.Type & MOVE_PAWN_PROMOTE) {
            Board->Pieces[To] = PIECE_CREATE(Info->PromotePieceType, BLACK);

            Board->BB_BlackPieces |= BB_SQUARE(To);
            Board->BB_Pieces[BLACK][Info->PromotePieceType] |= BB_SQUARE(To);

            Board->Hash ^= PieceHash[BLACK][Info->PromotePieceType][To];

            AccumulatorAdd(Board, To, PIECE_CREATE(Info->PromotePieceType, BLACK));
        }
        else {
            Board->Pieces[To] = Board->Pieces[From];

            Board->BB_BlackPieces |= BB_SQUARE(To);
            Board->BB_Pieces[BLACK][Info->PieceTypeFrom] |= BB_SQUARE(To);

            Board->Hash ^= PieceHash[BLACK][Info->PieceTypeFrom][To];

            AccumulatorAdd(Board, To, PIECE_CREATE(Info->PieceTypeFrom, BLACK));
        }

        Board->Pieces[From] = NO_PIECE;

        Board->BB_BlackPieces &= ~BB_SQUARE(From);
        Board->BB_Pieces[BLACK][Info->PieceTypeFrom] &= ~BB_SQUARE(From);

        Board->Hash ^= PieceHash[BLACK][Info->PieceTypeFrom][From];

        AccumulatorSub(Board, From, PIECE_CREATE(Info->PieceTypeFrom, BLACK));
    }

    if (Move.Type & (MOVE_CAPTURE | MOVE_PAWN | MOVE_PAWN_2)) {
        Board->FiftyMove = 0;
    }
    else {
        ++Board->FiftyMove;
    }

    Board->CurrentColor ^= 1;

    Board->Hash ^= ColorHash;

#ifdef DEBUG_NNUE
    I16 Accumulator[2][HIDDEN_DIMENSION]; // [Perspective][Hidden dimension]

    memcpy(Accumulator, Board->Accumulator, sizeof(Accumulator));

    InitAccumulator(Board);

    for (int Perspective = 0; Perspective < 2; ++Perspective) { // STM/XSTM
        for (int Index = 0; Index < HIDDEN_DIMENSION; ++Index) {
            if (Board->Accumulator[Perspective][Index] != Accumulator[Perspective][Index]) {
                printf("-- Accumulator error (make)! Color = %d Piece = %d From = %d To = %d Move type = %d\n", CHANGE_COLOR(Board->CurrentColor), Info->PieceTypeFrom, Info->From, Info->To, Info->Type);

                break;
            }
        }
    }
#endif // DEBUG_NNUE

#ifdef DEBUG_HASH
    U64 PreviousHash = Board->Hash;

    InitHash(Board);

    if (Board->Hash != PreviousHash) {
        printf("-- Board hash error! BoardHash = 0x%016llx PreviousHash = 0x%016llx\n", Board->Hash, PreviousHash);
    }
#endif // DEBUG_HASH
}

void UnmakeMove(BoardItem* Board)
{
    HistoryItem* Info = &Board->MoveTable[--Board->HalfMoveNumber];

    Board->CurrentColor ^= 1;

    if (Board->CurrentColor == WHITE) {
        Board->Pieces[Info->From] = PIECE_CREATE(Info->PieceTypeFrom, WHITE);

        Board->BB_WhitePieces |= BB_SQUARE(Info->From);
        Board->BB_Pieces[WHITE][Info->PieceTypeFrom] |= BB_SQUARE(Info->From);

        AccumulatorAdd(Board, Info->From, PIECE_CREATE(Info->PieceTypeFrom, WHITE));

        if (Info->Type & MOVE_CASTLE_KING) { // White O-O
            Board->Pieces[SQ_F1] = NO_PIECE;

            Board->BB_WhitePieces &= ~BB_SQUARE(SQ_F1);
            Board->BB_Pieces[WHITE][ROOK] &= ~BB_SQUARE(SQ_F1);

            AccumulatorSub(Board, SQ_F1, PIECE_CREATE(ROOK, WHITE));

            Board->Pieces[SQ_H1] = PIECE_CREATE(ROOK, WHITE);

            Board->BB_WhitePieces |= BB_SQUARE(SQ_H1);
            Board->BB_Pieces[WHITE][ROOK] |= BB_SQUARE(SQ_H1);

            AccumulatorAdd(Board, SQ_H1, PIECE_CREATE(ROOK, WHITE));
        }

        if (Info->Type & MOVE_CASTLE_QUEEN) { // White O-O-O
            Board->Pieces[SQ_D1] = NO_PIECE;

            Board->BB_WhitePieces &= ~BB_SQUARE(SQ_D1);
            Board->BB_Pieces[WHITE][ROOK] &= ~BB_SQUARE(SQ_D1);

            AccumulatorSub(Board, SQ_D1, PIECE_CREATE(ROOK, WHITE));

            Board->Pieces[SQ_A1] = PIECE_CREATE(ROOK, WHITE);

            Board->BB_WhitePieces |= BB_SQUARE(SQ_A1);
            Board->BB_Pieces[WHITE][ROOK] |= BB_SQUARE(SQ_A1);

            AccumulatorAdd(Board, SQ_A1, PIECE_CREATE(ROOK, WHITE));
        }

        if (Info->Type & MOVE_PAWN_EN_PASSANT) {
            Board->Pieces[Info->To] = NO_PIECE;

            Board->BB_WhitePieces &= ~BB_SQUARE(Info->To);
            Board->BB_Pieces[WHITE][PAWN] &= ~BB_SQUARE(Info->To);

            AccumulatorSub(Board, Info->To, PIECE_CREATE(PAWN, WHITE));

            Board->Pieces[Info->CapturePawnSquare] = PIECE_CREATE(PAWN, BLACK);

            Board->BB_BlackPieces |= BB_SQUARE(Info->CapturePawnSquare);
            Board->BB_Pieces[BLACK][PAWN] |= BB_SQUARE(Info->CapturePawnSquare);

            AccumulatorAdd(Board, Info->CapturePawnSquare, PIECE_CREATE(PAWN, BLACK));
        }
        else if (Info->Type & MOVE_CAPTURE) {
            Board->Pieces[Info->To] = NO_PIECE;

            Board->BB_WhitePieces &= ~BB_SQUARE(Info->To);

            if (Info->Type & MOVE_PAWN_PROMOTE) {
                Board->BB_Pieces[WHITE][Info->PromotePieceType] &= ~BB_SQUARE(Info->To);

                AccumulatorSub(Board, Info->To, PIECE_CREATE(Info->PromotePieceType, WHITE));
            }
            else {
                Board->BB_Pieces[WHITE][Info->PieceTypeFrom] &= ~BB_SQUARE(Info->To);

                AccumulatorSub(Board, Info->To, PIECE_CREATE(Info->PieceTypeFrom, WHITE));
            }

            Board->Pieces[Info->To] = PIECE_CREATE(Info->PieceTypeTo, BLACK);

            Board->BB_BlackPieces |= BB_SQUARE(Info->To);
            Board->BB_Pieces[BLACK][Info->PieceTypeTo] |= BB_SQUARE(Info->To);

            AccumulatorAdd(Board, Info->To, PIECE_CREATE(Info->PieceTypeTo, BLACK));
        }
        else {
            Board->Pieces[Info->To] = NO_PIECE;

            Board->BB_WhitePieces &= ~BB_SQUARE(Info->To);

            if (Info->Type & MOVE_PAWN_PROMOTE) {
                Board->BB_Pieces[WHITE][Info->PromotePieceType] &= ~BB_SQUARE(Info->To);

                AccumulatorSub(Board, Info->To, PIECE_CREATE(Info->PromotePieceType, WHITE));
            }
            else {
                Board->BB_Pieces[WHITE][Info->PieceTypeFrom] &= ~BB_SQUARE(Info->To);

                AccumulatorSub(Board, Info->To, PIECE_CREATE(Info->PieceTypeFrom, WHITE));
            }
        }
    }
    else { // BLACK
        Board->Pieces[Info->From] = PIECE_CREATE(Info->PieceTypeFrom, BLACK);

        Board->BB_BlackPieces |= BB_SQUARE(Info->From);
        Board->BB_Pieces[BLACK][Info->PieceTypeFrom] |= BB_SQUARE(Info->From);

        AccumulatorAdd(Board, Info->From, PIECE_CREATE(Info->PieceTypeFrom, BLACK));

        if (Info->Type & MOVE_CASTLE_KING) { // Black O-O
            Board->Pieces[SQ_F8] = NO_PIECE;

            Board->BB_BlackPieces &= ~BB_SQUARE(SQ_F8);
            Board->BB_Pieces[BLACK][ROOK] &= ~BB_SQUARE(SQ_F8);

            AccumulatorSub(Board, SQ_F8, PIECE_CREATE(ROOK, BLACK));

            Board->Pieces[SQ_H8] = PIECE_CREATE(ROOK, BLACK);

            Board->BB_BlackPieces |= BB_SQUARE(SQ_H8);
            Board->BB_Pieces[BLACK][ROOK] |= BB_SQUARE(SQ_H8);

            AccumulatorAdd(Board, SQ_H8, PIECE_CREATE(ROOK, BLACK));
        }

        if (Info->Type & MOVE_CASTLE_QUEEN) { // Black O-O-O
            Board->Pieces[SQ_D8] = NO_PIECE;

            Board->BB_BlackPieces &= ~BB_SQUARE(SQ_D8);
            Board->BB_Pieces[BLACK][ROOK] &= ~BB_SQUARE(SQ_D8);

            AccumulatorSub(Board, SQ_D8, PIECE_CREATE(ROOK, BLACK));

            Board->Pieces[SQ_A8] = PIECE_CREATE(ROOK, BLACK);

            Board->BB_BlackPieces |= BB_SQUARE(SQ_A8);
            Board->BB_Pieces[BLACK][ROOK] |= BB_SQUARE(SQ_A8);

            AccumulatorAdd(Board, SQ_A8, PIECE_CREATE(ROOK, BLACK));
        }

        if (Info->Type & MOVE_PAWN_EN_PASSANT) {
            Board->Pieces[Info->To] = NO_PIECE;

            Board->BB_BlackPieces &= ~BB_SQUARE(Info->To);
            Board->BB_Pieces[BLACK][PAWN] &= ~BB_SQUARE(Info->To);

            AccumulatorSub(Board, Info->To, PIECE_CREATE(PAWN, BLACK));

            Board->Pieces[Info->CapturePawnSquare] = PIECE_CREATE(PAWN, WHITE);

            Board->BB_WhitePieces |= BB_SQUARE(Info->CapturePawnSquare);
            Board->BB_Pieces[WHITE][PAWN] |= BB_SQUARE(Info->CapturePawnSquare);

            AccumulatorAdd(Board, Info->CapturePawnSquare, PIECE_CREATE(PAWN, WHITE));
        }
        else if (Info->Type & MOVE_CAPTURE) {
            Board->Pieces[Info->To] = NO_PIECE;

            Board->BB_BlackPieces &= ~BB_SQUARE(Info->To);

            if (Info->Type & MOVE_PAWN_PROMOTE) {
                Board->BB_Pieces[BLACK][Info->PromotePieceType] &= ~BB_SQUARE(Info->To);

                AccumulatorSub(Board, Info->To, PIECE_CREATE(Info->PromotePieceType, BLACK));
            }
            else {
                Board->BB_Pieces[BLACK][Info->PieceTypeFrom] &= ~BB_SQUARE(Info->To);

                AccumulatorSub(Board, Info->To, PIECE_CREATE(Info->PieceTypeFrom, BLACK));
            }

            Board->Pieces[Info->To] = PIECE_CREATE(Info->PieceTypeTo, WHITE);

            Board->BB_WhitePieces |= BB_SQUARE(Info->To);
            Board->BB_Pieces[WHITE][Info->PieceTypeTo] |= BB_SQUARE(Info->To);

            AccumulatorAdd(Board, Info->To, PIECE_CREATE(Info->PieceTypeTo, WHITE));
        }
        else {
            Board->Pieces[Info->To] = NO_PIECE;

            Board->BB_BlackPieces &= ~BB_SQUARE(Info->To);

            if (Info->Type & MOVE_PAWN_PROMOTE) {
                Board->BB_Pieces[BLACK][Info->PromotePieceType] &= ~BB_SQUARE(Info->To);

                AccumulatorSub(Board, Info->To, PIECE_CREATE(Info->PromotePieceType, BLACK));
            }
            else {
                Board->BB_Pieces[BLACK][Info->PieceTypeFrom] &= ~BB_SQUARE(Info->To);

                AccumulatorSub(Board, Info->To, PIECE_CREATE(Info->PieceTypeFrom, BLACK));
            }
        }
    }

    Board->EnPassantSquare = Info->EnPassantSquare;

    Board->CastleFlags = Info->CastleFlags;
    Board->FiftyMove = Info->FiftyMove;

    Board->Hash = Info->Hash;

#ifdef DEBUG_NNUE
    I16 Accumulator[2][HIDDEN_DIMENSION]; // [Perspective][Hidden dimension]

    memcpy(Accumulator, Board->Accumulator, sizeof(Accumulator));

    InitAccumulator(Board);

    for (int Perspective = 0; Perspective < 2; ++Perspective) { // STM/XSTM
        for (int Index = 0; Index < HIDDEN_DIMENSION; ++Index) {
            if (Board->Accumulator[Perspective][Index] != Accumulator[Perspective][Index]) {
                printf("-- Accumulator error (unmake)! Color = %d Piece = %d From = %d To = %d Move type = %d\n", CHANGE_COLOR(Board->CurrentColor), Info->PieceTypeFrom, Info->From, Info->To, Info->Type);

                break;
            }
        }
    }
#endif // DEBUG_NNUE

#ifdef DEBUG_MOVE
    if (Board->BB_WhitePieces != Info->BB_WhitePieces) {
        printf("-- BB_WhitePieces error! From = %d To = %d Move type = %d\n", Info->From, Info->To, Info->Type);
    }

    if (Board->BB_BlackPieces != Info->BB_BlackPieces) {
        printf("-- BB_BlackPieces error! From = %d To = %d Move type = %d\n", Info->From, Info->To, Info->Type);
    }

    for (int Color = 0; Color < 2; ++Color) { // White/Black
        for (int Piece = 0; Piece < 6; ++Piece) { // PNBRQK
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

    Info->EnPassantSquare = Board->EnPassantSquare;

    Info->FiftyMove = Board->FiftyMove;

    Info->Hash = Board->Hash;

    if (Info->EnPassantSquare != -1) {
        Board->EnPassantSquare = -1;

        Board->Hash ^= PassantHash[Info->EnPassantSquare];
    }

    ++Board->FiftyMove;

    Board->CurrentColor ^= 1;

    Board->Hash ^= ColorHash;

#ifdef DEBUG_HASH
    U64 PreviousHash = Board->Hash;

    InitHash(Board);

    if (Board->Hash != PreviousHash) {
        printf("-- Board hash error! BoardHash = 0x%016llx PreviousHash = 0x%016llx\n", Board->Hash, PreviousHash);
    }
#endif // DEBUG_HASH
}

void UnmakeNullMove(BoardItem* Board)
{
    HistoryItem* Info = &Board->MoveTable[--Board->HalfMoveNumber];

    Board->CurrentColor ^= 1;

    Board->EnPassantSquare = Info->EnPassantSquare;

    Board->FiftyMove = Info->FiftyMove;

    Board->Hash = Info->Hash;
}

#endif // NULL_MOVE_PRUNING