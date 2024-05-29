// Evaluate.h

#pragma once

#ifndef EVALUATE_H
#define EVALUATE_H

#include "Board.h"
#include "Def.h"
#include "Types.h"

// Game phase (4.1)

extern const int MaxGamePhase;

// Material (4.2)

extern int PiecesScoreOpening[6];
extern int PiecesScoreEnding[6];

extern int BishopPairOpening;
extern int BishopPairEnding;

// Piece-square tables (4.3)

extern int PawnSquareScoreOpening[64];
extern int PawnSquareScoreEnding[64];

extern int KnightSquareScoreOpening[64];
extern int KnightSquareScoreEnding[64];

extern int BishopSquareScoreOpening[64];
extern int BishopSquareScoreEnding[64];

extern int RookSquareScoreOpening[64];
extern int RookSquareScoreEnding[64];

extern int QueenSquareScoreOpening[64];
extern int QueenSquareScoreEnding[64];

extern int KingSquareScoreOpening[64];
extern int KingSquareScoreEnding[64];

// Pawns (4.4)

extern int PawnDoubledOpening;
extern int PawnDoubledEnding;

extern int PawnIsolatedOpening;
extern int PawnIsolatedEnding;

extern int PawnIsolatedOpenOpening;
extern int PawnIsolatedOpenEnding;

extern int PawnBackwardOpening;
extern int PawnBackwardEnding;

extern int PawnBackwardOpenOpening;
extern int PawnBackwardOpenEnding;

extern int PawnCandidateOpening[8];
extern int PawnCandidateEnding[8];

// Tempo (4.5)

extern int TempoOpening;
extern int TempoEnding;

// Pattern (4.6)

extern int TrappedBishopOpening;
extern int TrappedBishopEnding;

extern int BlockedBishopOpening;
extern int BlockedBishopEnding;

extern int BlockedRookOpening;
extern int BlockedRookEnding;

// Piece (4.7)

// Mobility (4.7.1)

extern int KnightMobilityMoveDecrease;
extern int BishopMobilityMoveDecrease;
extern int RookMobilityMoveDecrease;

extern int KnightMobility;
extern int BishopMobility;

extern int RookMobilityOpening;
extern int RookMobilityEnding;

// Open file (4.7.2)

extern int RookOnClosedFileOpening;
extern int RookOnClosedFileEnding;

extern int RookOnSemiOpenFileOpening;
extern int RookOnSemiOpenFileEnding;

extern int RookOnSemiOpenFileAdjacentToEnemyKingOpening;
extern int RookOnSemiOpenFileAdjacentToEnemyKingEnding;

extern int RookOnSemiOpenFileSameToEnemyKingOpening;
extern int RookOnSemiOpenFileSameToEnemyKingEnding;

extern int RookOnOpenFileOpening;
extern int RookOnOpenFileEnding;

extern int RookOnOpenFileAdjacentToEnemyKingOpening;
extern int RookOnOpenFileAdjacentToEnemyKingEnding;

extern int RookOnOpenFileSameToEnemyKingOpening;
extern int RookOnOpenFileSameToEnemyKingEnding;

// Outpost (4.7.3)

extern int KnightOutpost[64];

// Seventh rank (4.7.4)

extern int RookOnSeventhOpening;
extern int RookOnSeventhEnding;

extern int QueenOnSeventhOpening;
extern int QueenOnSeventhEnding;

// King distance (4.7.5)

// King (4.8)

// Pawn shelter (4.8.1)

extern int KingFriendlyPawnAdvanceBase;
extern int KingFriendlyPawnAdvanceDefault;

// Pawn storm (4.8.2)

extern int KingPawnStorm[8];

// Piece attack (4.8.3)

extern int KingZoneAttackedBase;
extern int KingZoneAttackedWeight[8]; // %

// Passed pawns (4.9)

extern int PawnPassedEndingBase1;
extern int PawnPassedEndingBase2;

extern int PawnPassedEndingHostileKingDistance;
extern int PawnPassedEndingFriendlyKingDistance;

extern int PawnPassedEndingConsideredFree;

extern int PawnPassedEndingUnstoppable;

extern int PawnPassedOpening[8];
extern int PawnPassedEndingWeight[8]; // %

// ------------------------------------------------------------------

void InitEvaluation(void);

int Evaluate(BoardItem* Board);

#endif // !EVALUATE_H