// NNUE2.cpp

#include "stdafx.h"

#include "NNUE2.h"

#include "BitBoard.h"
#include "Board.h"
#include "Def.h"
#include "Types.h"
#include "Utils.h"

#define NNUE_FILE_MAGIC             ('B' | 'R' << 8 | 'K' << 16 | 'R' << 24)
#define NNUE_FILE_SIZE              (4 + 8 + (INPUT_DIMENSION * HIDDEN_DIMENSION + HIDDEN_DIMENSION + HIDDEN_DIMENSION * 2 + 1) * 4)

#define NNUE_HASH(Hash, Value)      (((Hash) + 324723947ULL + (Value)) ^ 93485734985ULL)

#define STM                         0
#define XSTM                        1

#ifdef USE_NNUE_AVX2
#define NUM_REGS                    (HIDDEN_DIMENSION * sizeof(I16) / sizeof(__m256i))
#endif // USE_NNUE_AVX2

_declspec(align(64)) I16 InputWeights[INPUT_DIMENSION * HIDDEN_DIMENSION];
_declspec(align(64)) I16 InputBiases[HIDDEN_DIMENSION];
_declspec(align(64)) I16 OutputWeights[HIDDEN_DIMENSION * 2];
I32 OutputBias;

BOOL NnueFileLoaded = FALSE;

I16 LoadInt16(const float Value, const int Precision)
{
    return (I16)roundf(Value * (float)Precision);
}

I32 LoadInt32(const float Value, const int Precision)
{
    return (I32)roundf(Value * (float)Precision);
}

void LoadNetwork(const char* NnueFileName)
{
    printf("\n");
    printf("Load network...\n");

    NnueFileLoaded = FALSE; // The network may have been loaded earlier

    // Open file

    FILE* File;

    fopen_s(&File, NnueFileName, "rb");

    if (File == NULL) { // File open error
        printf("File '%s' open error!\n", NnueFileName);

        return;
    }

    // Read and check file magic

    int FileMagic;

    fread(&FileMagic, 4, 1, File);

    if (FileMagic != NNUE_FILE_MAGIC) { // File format error
        printf("File '%s' format error!\n", NnueFileName);

        fclose(File);

        return;
    }

    // Read file hash

    U64 FileHash;

    fread(&FileHash, sizeof(U64), 1, File);

    // Read weights and biases; calculate file hash

    float Value;

    U64 NnueHash = 0ULL;

    // Read input weights

    for (int Index = 0; Index < INPUT_DIMENSION * HIDDEN_DIMENSION; Index++) {
        fread(&Value, sizeof(float), 1, File);

        NnueHash = NNUE_HASH(NnueHash, (U64)Value);

        InputWeights[Index] = LoadInt16(Value, QUANTIZATION_PRECISION_IN);
    }

    // Read input biases

    for (int Index = 0; Index < HIDDEN_DIMENSION; Index++) {
        fread(&Value, sizeof(float), 1, File);

        NnueHash = NNUE_HASH(NnueHash, (U64)Value);

        InputBiases[Index] = LoadInt16(Value, QUANTIZATION_PRECISION_IN);
    }

    // Read output weights

    for (int Index = 0; Index < HIDDEN_DIMENSION * 2; Index++) {
        fread(&Value, sizeof(float), 1, File);

        NnueHash = NNUE_HASH(NnueHash, (U64)Value);

        OutputWeights[Index] = LoadInt16(Value, QUANTIZATION_PRECISION_OUT);
    }

    // Read output bias

    fread(&Value, sizeof(float), 1, File);

    NnueHash = NNUE_HASH(NnueHash, (U64)Value);

    OutputBias = LoadInt32(Value, QUANTIZATION_PRECISION_IN * QUANTIZATION_PRECISION_OUT);

    // Check file size (part 1)

    int FilePos1 = ftell(File);

    if (FilePos1 != NNUE_FILE_SIZE) { // File format error
        printf("File '%s' format error!\n", NnueFileName);

        fclose(File);

        return;
    }

    // Check file size (part 2)

    fseek(File, 0, SEEK_END);

    int FilePos2 = ftell(File);

    if (FilePos2 != NNUE_FILE_SIZE) { // File format error
        printf("File '%s' format error!\n", NnueFileName);

        fclose(File);

        return;
    }

    // Check file hash

    if (FileHash != NnueHash) { // File format error
        printf("File '%s' format error!\n", NnueFileName);

        fclose(File);

        return;
    }

    // Close file

    fclose(File);

    NnueFileLoaded = TRUE;

    printf("Load network...DONE (%s; 0x%012llx)\n", NnueFileName, FileHash);
}

BOOL IsNetworkLoaded(void)
{
    return NnueFileLoaded;
}

int CalculateWeightIndex(const int Perspective, const int Square, const int PieceWithColor)
{
    int PieceIndex;
    int WeightIndex;

    if (Perspective == STM) {
        PieceIndex = PIECE_TYPE(PieceWithColor) + 6 * PIECE_COLOR(PieceWithColor);
        WeightIndex = (PieceIndex << 6) + Square;
    }
    else { // XSTM
        PieceIndex = PIECE_TYPE(PieceWithColor) + 6 * CHANGE_COLOR(PIECE_COLOR(PieceWithColor));
        WeightIndex = (PieceIndex << 6) + (Square ^ 56);
    }

    return WeightIndex;
}

void AccumulatorAdd1Sub1(BoardItem* Board, int Square1, int PieceWithColor1, int Square2, int PieceWithColor2)
{
    for (int Perspective = 0; Perspective < 2; Perspective++) { // STM/XSTM
        int WeightIndex1 = CalculateWeightIndex(Perspective, Square1, PieceWithColor1);
        int WeightIndex2 = CalculateWeightIndex(Perspective, Square2, PieceWithColor2);

#ifdef USE_NNUE_AVX2
        __m256i* __restrict Accumulator = (__m256i*)Board->Accumulator.Accumulator[Perspective];

        const __m256i* __restrict Weights1 = (__m256i*)&InputWeights[WeightIndex1 * HIDDEN_DIMENSION];
        const __m256i* __restrict Weights2 = (__m256i*)&InputWeights[WeightIndex2 * HIDDEN_DIMENSION];

        for (int Reg = 0; Reg < NUM_REGS; Reg += 4) {
            __m256i Accumulator0 = _mm256_load_si256(&Accumulator[Reg + 0]);
            __m256i Accumulator1 = _mm256_load_si256(&Accumulator[Reg + 1]);
            __m256i Accumulator2 = _mm256_load_si256(&Accumulator[Reg + 2]);
            __m256i Accumulator3 = _mm256_load_si256(&Accumulator[Reg + 3]);

            __m256i Diff0 = _mm256_sub_epi16(_mm256_load_si256(&Weights1[Reg + 0]), _mm256_load_si256(&Weights2[Reg + 0]));
            __m256i Diff1 = _mm256_sub_epi16(_mm256_load_si256(&Weights1[Reg + 1]), _mm256_load_si256(&Weights2[Reg + 1]));
            __m256i Diff2 = _mm256_sub_epi16(_mm256_load_si256(&Weights1[Reg + 2]), _mm256_load_si256(&Weights2[Reg + 2]));
            __m256i Diff3 = _mm256_sub_epi16(_mm256_load_si256(&Weights1[Reg + 3]), _mm256_load_si256(&Weights2[Reg + 3]));

            _mm256_store_si256(&Accumulator[Reg + 0], _mm256_add_epi16(Accumulator0, Diff0));
            _mm256_store_si256(&Accumulator[Reg + 1], _mm256_add_epi16(Accumulator1, Diff1));
            _mm256_store_si256(&Accumulator[Reg + 2], _mm256_add_epi16(Accumulator2, Diff2));
            _mm256_store_si256(&Accumulator[Reg + 3], _mm256_add_epi16(Accumulator3, Diff3));
        }
#else
        I16* __restrict Accumulator = Board->Accumulator.Accumulator[Perspective];

        const I16* __restrict Weights1 = &InputWeights[WeightIndex1 * HIDDEN_DIMENSION];
        const I16* __restrict Weights2 = &InputWeights[WeightIndex2 * HIDDEN_DIMENSION];

        for (int Index = 0; Index < HIDDEN_DIMENSION; Index++) {
            Accumulator[Index] += Weights1[Index] - Weights2[Index];
        }
#endif // USE_NNUE_AVX2
    }
}

void AccumulatorAdd1Sub2(BoardItem* Board, int Square1, int PieceWithColor1, int Square2, int PieceWithColor2, int Square3, int PieceWithColor3)
{
    for (int Perspective = 0; Perspective < 2; Perspective++) { // STM/XSTM
        int WeightIndex1 = CalculateWeightIndex(Perspective, Square1, PieceWithColor1);
        int WeightIndex2 = CalculateWeightIndex(Perspective, Square2, PieceWithColor2);
        int WeightIndex3 = CalculateWeightIndex(Perspective, Square3, PieceWithColor3);

#ifdef USE_NNUE_AVX2
        __m256i* __restrict Accumulator = (__m256i*)Board->Accumulator.Accumulator[Perspective];

        const __m256i* __restrict Weights1 = (__m256i*)&InputWeights[WeightIndex1 * HIDDEN_DIMENSION];
        const __m256i* __restrict Weights2 = (__m256i*)&InputWeights[WeightIndex2 * HIDDEN_DIMENSION];
        const __m256i* __restrict Weights3 = (__m256i*)&InputWeights[WeightIndex3 * HIDDEN_DIMENSION];

        for (int Reg = 0; Reg < NUM_REGS; Reg += 4) {
            __m256i Add0 = _mm256_add_epi16(_mm256_load_si256(&Accumulator[Reg + 0]), _mm256_load_si256(&Weights1[Reg + 0]));
            __m256i Add1 = _mm256_add_epi16(_mm256_load_si256(&Accumulator[Reg + 1]), _mm256_load_si256(&Weights1[Reg + 1]));
            __m256i Add2 = _mm256_add_epi16(_mm256_load_si256(&Accumulator[Reg + 2]), _mm256_load_si256(&Weights1[Reg + 2]));
            __m256i Add3 = _mm256_add_epi16(_mm256_load_si256(&Accumulator[Reg + 3]), _mm256_load_si256(&Weights1[Reg + 3]));

            __m256i Sub0 = _mm256_add_epi16(_mm256_load_si256(&Weights2[Reg + 0]), _mm256_load_si256(&Weights3[Reg + 0]));
            __m256i Sub1 = _mm256_add_epi16(_mm256_load_si256(&Weights2[Reg + 1]), _mm256_load_si256(&Weights3[Reg + 1]));
            __m256i Sub2 = _mm256_add_epi16(_mm256_load_si256(&Weights2[Reg + 2]), _mm256_load_si256(&Weights3[Reg + 2]));
            __m256i Sub3 = _mm256_add_epi16(_mm256_load_si256(&Weights2[Reg + 3]), _mm256_load_si256(&Weights3[Reg + 3]));

            _mm256_store_si256(&Accumulator[Reg + 0], _mm256_sub_epi16(Add0, Sub0));
            _mm256_store_si256(&Accumulator[Reg + 1], _mm256_sub_epi16(Add1, Sub1));
            _mm256_store_si256(&Accumulator[Reg + 2], _mm256_sub_epi16(Add2, Sub2));
            _mm256_store_si256(&Accumulator[Reg + 3], _mm256_sub_epi16(Add3, Sub3));
        }
#else
        I16* __restrict Accumulator = Board->Accumulator.Accumulator[Perspective];

        const I16* __restrict Weights1 = &InputWeights[WeightIndex1 * HIDDEN_DIMENSION];
        const I16* __restrict Weights2 = &InputWeights[WeightIndex2 * HIDDEN_DIMENSION];
        const I16* __restrict Weights3 = &InputWeights[WeightIndex3 * HIDDEN_DIMENSION];

        for (int Index = 0; Index < HIDDEN_DIMENSION; Index++) {
            Accumulator[Index] += Weights1[Index] - (Weights2[Index] + Weights3[Index]);
        }
#endif // USE_NNUE_AVX2
    }
}

void AccumulatorAdd2Sub2(BoardItem* Board, int Square1, int PieceWithColor1, int Square2, int PieceWithColor2, int Square3, int PieceWithColor3, int Square4, int PieceWithColor4)
{
    for (int Perspective = 0; Perspective < 2; Perspective++) { // STM/XSTM
        int WeightIndex1 = CalculateWeightIndex(Perspective, Square1, PieceWithColor1);
        int WeightIndex2 = CalculateWeightIndex(Perspective, Square2, PieceWithColor2);
        int WeightIndex3 = CalculateWeightIndex(Perspective, Square3, PieceWithColor3);
        int WeightIndex4 = CalculateWeightIndex(Perspective, Square4, PieceWithColor4);

#ifdef USE_NNUE_AVX2
        __m256i* __restrict Accumulator = (__m256i*)Board->Accumulator.Accumulator[Perspective];

        const __m256i* __restrict Weights1 = (__m256i*)&InputWeights[WeightIndex1 * HIDDEN_DIMENSION];
        const __m256i* __restrict Weights2 = (__m256i*)&InputWeights[WeightIndex2 * HIDDEN_DIMENSION];
        const __m256i* __restrict Weights3 = (__m256i*)&InputWeights[WeightIndex3 * HIDDEN_DIMENSION];
        const __m256i* __restrict Weights4 = (__m256i*)&InputWeights[WeightIndex4 * HIDDEN_DIMENSION];

        for (int Reg = 0; Reg < NUM_REGS; Reg += 4) {
            __m256i Add0 = _mm256_add_epi16(_mm256_load_si256(&Weights1[Reg + 0]), _mm256_load_si256(&Weights3[Reg + 0]));
            __m256i Add1 = _mm256_add_epi16(_mm256_load_si256(&Weights1[Reg + 1]), _mm256_load_si256(&Weights3[Reg + 1]));
            __m256i Add2 = _mm256_add_epi16(_mm256_load_si256(&Weights1[Reg + 2]), _mm256_load_si256(&Weights3[Reg + 2]));
            __m256i Add3 = _mm256_add_epi16(_mm256_load_si256(&Weights1[Reg + 3]), _mm256_load_si256(&Weights3[Reg + 3]));

            __m256i Sub0 = _mm256_add_epi16(_mm256_load_si256(&Weights2[Reg + 0]), _mm256_load_si256(&Weights4[Reg + 0]));
            __m256i Sub1 = _mm256_add_epi16(_mm256_load_si256(&Weights2[Reg + 1]), _mm256_load_si256(&Weights4[Reg + 1]));
            __m256i Sub2 = _mm256_add_epi16(_mm256_load_si256(&Weights2[Reg + 2]), _mm256_load_si256(&Weights4[Reg + 2]));
            __m256i Sub3 = _mm256_add_epi16(_mm256_load_si256(&Weights2[Reg + 3]), _mm256_load_si256(&Weights4[Reg + 3]));

            __m256i Diff0 = _mm256_sub_epi16(Add0, Sub0);
            __m256i Diff1 = _mm256_sub_epi16(Add1, Sub1);
            __m256i Diff2 = _mm256_sub_epi16(Add2, Sub2);
            __m256i Diff3 = _mm256_sub_epi16(Add3, Sub3);

            _mm256_store_si256(&Accumulator[Reg + 0], _mm256_add_epi16(_mm256_load_si256(&Accumulator[Reg + 0]), Diff0));
            _mm256_store_si256(&Accumulator[Reg + 1], _mm256_add_epi16(_mm256_load_si256(&Accumulator[Reg + 1]), Diff1));
            _mm256_store_si256(&Accumulator[Reg + 2], _mm256_add_epi16(_mm256_load_si256(&Accumulator[Reg + 2]), Diff2));
            _mm256_store_si256(&Accumulator[Reg + 3], _mm256_add_epi16(_mm256_load_si256(&Accumulator[Reg + 3]), Diff3));
        }
#else
        I16* __restrict Accumulator = Board->Accumulator.Accumulator[Perspective];

        const I16* __restrict Weights1 = &InputWeights[WeightIndex1 * HIDDEN_DIMENSION];
        const I16* __restrict Weights2 = &InputWeights[WeightIndex2 * HIDDEN_DIMENSION];
        const I16* __restrict Weights3 = &InputWeights[WeightIndex3 * HIDDEN_DIMENSION];
        const I16* __restrict Weights4 = &InputWeights[WeightIndex4 * HIDDEN_DIMENSION];

        for (int Index = 0; Index < HIDDEN_DIMENSION; Index++) {
            Accumulator[Index] += (Weights1[Index] + Weights3[Index]) - (Weights2[Index] + Weights4[Index]);
        }
#endif // USE_NNUE_AVX2
    }
}

void InitAccumulator(BoardItem* Board)
{
    const I16* WeightSTM[32];
    const I16* WeightXSTM[32];

    int PieceCount = 0;

    U64 Pieces = (Board->BB_WhitePieces | Board->BB_BlackPieces);

    while (Pieces) {
        int Square = LSB(Pieces);
        int PieceWithColor = Board->Pieces[Square];

        int WeightIndexSTM = CalculateWeightIndex(STM, Square, PieceWithColor);
        int WeightIndexXSTM = CalculateWeightIndex(XSTM, Square, PieceWithColor);

        WeightSTM[PieceCount] = &InputWeights[WeightIndexSTM * HIDDEN_DIMENSION];
        WeightXSTM[PieceCount] = &InputWeights[WeightIndexXSTM * HIDDEN_DIMENSION];

        PieceCount++;

        Pieces &= Pieces - 1ULL;
    }

#ifdef USE_NNUE_AVX2
    const __m256i* __restrict Biases = (__m256i*)InputBiases;

    __m256i* __restrict AccumulatorSTM = (__m256i*)Board->Accumulator.Accumulator[STM];
    __m256i* __restrict AccumulatorXSTM = (__m256i*)Board->Accumulator.Accumulator[XSTM];

    for (int Reg = 0; Reg < NUM_REGS; Reg += 8) {
        __m256i SumSTM[8];
        __m256i SumXSTM[8];

        for (int Index = 0; Index < 8; Index++) {
            SumSTM[Index] = _mm256_load_si256(&Biases[Reg + Index]);
            SumXSTM[Index] = SumSTM[Index];
        }

        for (int Piece = 0; Piece < PieceCount; Piece++) {
            const __m256i* WeightsSTM = (__m256i*)WeightSTM[Piece] + Reg;
            const __m256i* WeightsXSTM = (__m256i*)WeightXSTM[Piece] + Reg;

            for (int Index = 0; Index < 8; Index++) {
                SumSTM[Index] = _mm256_add_epi16(SumSTM[Index], _mm256_load_si256(&WeightsSTM[Index]));
                SumXSTM[Index] = _mm256_add_epi16(SumXSTM[Index], _mm256_load_si256(&WeightsXSTM[Index]));
            }
        }

        for (int Index = 0; Index < 8; Index++) {
            _mm256_store_si256(&AccumulatorSTM[Reg + Index], SumSTM[Index]);
            _mm256_store_si256(&AccumulatorXSTM[Reg + Index], SumXSTM[Index]);
        }
    }
#else
    memcpy(Board->Accumulator.Accumulator[STM], InputBiases, sizeof(Board->Accumulator.Accumulator[STM]));
    memcpy(Board->Accumulator.Accumulator[XSTM], InputBiases, sizeof(Board->Accumulator.Accumulator[XSTM]));

    for (int Piece = 0; Piece < PieceCount; Piece++) {
        for (int Index = 0; Index < HIDDEN_DIMENSION; Index++) {
            Board->Accumulator.Accumulator[STM][Index] += WeightSTM[Piece][Index];
            Board->Accumulator.Accumulator[XSTM][Index] += WeightXSTM[Piece][Index];
        }
    }
#endif // USE_NNUE_AVX2

    Board->Accumulator.AccumulatorActual = TRUE;
}

BOOL UpdateAccumulator(BoardItem* Board)
{
    int UpdateMoves = 0;
    int ActualMoveIndex = -1;

    for (int MoveIndex = Board->HalfMoveNumber - 1; MoveIndex >= 0; MoveIndex--) {
        UpdateMoves++;

        HistoryItem* Info = &Board->MoveTable[MoveIndex];

        if (Info->Accumulator.AccumulatorActual) {
            ActualMoveIndex = MoveIndex;

            break;
        }

        if (UpdateMoves >= MAX_UPDATE_MOVES) {
            ActualMoveIndex = -1;

            break;
        }
    }

    if (ActualMoveIndex == -1) {
        return FALSE;
    }

    int Color = Board->CurrentColor;

    for (int MoveIndex = Board->HalfMoveNumber - 1; MoveIndex >= ActualMoveIndex; MoveIndex--) {
        Color ^= 1;

        HistoryItem* Info = &Board->MoveTable[MoveIndex];

        if (Info->Type & MOVE_NULL) {
            continue;
        }

        if (Color == WHITE) {
            if (Info->Type & MOVE_CASTLE_KING) {
                AccumulatorAdd2Sub2(Board, Info->To, PIECE_CREATE(KING, WHITE), Info->From, PIECE_CREATE(KING, WHITE), SQ_F1, PIECE_CREATE(ROOK, WHITE), SQ_H1, PIECE_CREATE(ROOK, WHITE));
            }
            else if (Info->Type & MOVE_CASTLE_QUEEN) {
                AccumulatorAdd2Sub2(Board, Info->To, PIECE_CREATE(KING, WHITE), Info->From, PIECE_CREATE(KING, WHITE), SQ_D1, PIECE_CREATE(ROOK, WHITE), SQ_A1, PIECE_CREATE(ROOK, WHITE));
            }
            else if (Info->Type & MOVE_PAWN_EN_PASSANT) {
                AccumulatorAdd1Sub2(Board, Info->To, PIECE_CREATE(PAWN, WHITE), Info->From, PIECE_CREATE(PAWN, WHITE), Info->CapturePawnSquare, PIECE_CREATE(PAWN, BLACK));
            }
            else if (Info->Type & MOVE_CAPTURE) {
                if (Info->Type & MOVE_PAWN_PROMOTE) {
                    AccumulatorAdd1Sub2(Board, Info->To, PIECE_CREATE(Info->PromotePieceType, WHITE), Info->From, PIECE_CREATE(Info->PieceTypeFrom, WHITE), Info->To, PIECE_CREATE(Info->PieceTypeTo, BLACK));
                }
                else {
                    AccumulatorAdd1Sub2(Board, Info->To, PIECE_CREATE(Info->PieceTypeFrom, WHITE), Info->From, PIECE_CREATE(Info->PieceTypeFrom, WHITE), Info->To, PIECE_CREATE(Info->PieceTypeTo, BLACK));
                }
            }
            else {
                if (Info->Type & MOVE_PAWN_PROMOTE) {
                    AccumulatorAdd1Sub1(Board, Info->To, PIECE_CREATE(Info->PromotePieceType, WHITE), Info->From, PIECE_CREATE(Info->PieceTypeFrom, WHITE));
                }
                else {
                    AccumulatorAdd1Sub1(Board, Info->To, PIECE_CREATE(Info->PieceTypeFrom, WHITE), Info->From, PIECE_CREATE(Info->PieceTypeFrom, WHITE));
                }
            }
        }
        else { // BLACK
            if (Info->Type & MOVE_CASTLE_KING) {
                AccumulatorAdd2Sub2(Board, Info->To, PIECE_CREATE(KING, BLACK), Info->From, PIECE_CREATE(KING, BLACK), SQ_F8, PIECE_CREATE(ROOK, BLACK), SQ_H8, PIECE_CREATE(ROOK, BLACK));
            }
            else if (Info->Type & MOVE_CASTLE_QUEEN) {
                AccumulatorAdd2Sub2(Board, Info->To, PIECE_CREATE(KING, BLACK), Info->From, PIECE_CREATE(KING, BLACK), SQ_D8, PIECE_CREATE(ROOK, BLACK), SQ_A8, PIECE_CREATE(ROOK, BLACK));
            }
            else if (Info->Type & MOVE_PAWN_EN_PASSANT) {
                AccumulatorAdd1Sub2(Board, Info->To, PIECE_CREATE(PAWN, BLACK), Info->From, PIECE_CREATE(PAWN, BLACK), Info->CapturePawnSquare, PIECE_CREATE(PAWN, WHITE));
            }
            else if (Info->Type & MOVE_CAPTURE) {
                if (Info->Type & MOVE_PAWN_PROMOTE) {
                    AccumulatorAdd1Sub2(Board, Info->To, PIECE_CREATE(Info->PromotePieceType, BLACK), Info->From, PIECE_CREATE(Info->PieceTypeFrom, BLACK), Info->To, PIECE_CREATE(Info->PieceTypeTo, WHITE));
                }
                else {
                    AccumulatorAdd1Sub2(Board, Info->To, PIECE_CREATE(Info->PieceTypeFrom, BLACK), Info->From, PIECE_CREATE(Info->PieceTypeFrom, BLACK), Info->To, PIECE_CREATE(Info->PieceTypeTo, WHITE));
                }
            }
            else {
                if (Info->Type & MOVE_PAWN_PROMOTE) {
                    AccumulatorAdd1Sub1(Board, Info->To, PIECE_CREATE(Info->PromotePieceType, BLACK), Info->From, PIECE_CREATE(Info->PieceTypeFrom, BLACK));
                }
                else {
                    AccumulatorAdd1Sub1(Board, Info->To, PIECE_CREATE(Info->PieceTypeFrom, BLACK), Info->From, PIECE_CREATE(Info->PieceTypeFrom, BLACK));
                }
            }
        }
    } // for

#ifdef DEBUG_NNUE_UPDATE
    AccumulatorItem Accumulator = Board->Accumulator;

    InitAccumulator(Board);

    for (int Perspective = 0; Perspective < 2; Perspective++) { // STM/XSTM
        for (int Index = 0; Index < HIDDEN_DIMENSION; Index++) {
            if (Board->Accumulator.Accumulator[Perspective][Index] != Accumulator.Accumulator[Perspective][Index]) {
                printf("-- Accumulator error (update)!\n");

                break;
            }
        }
    }
#endif // DEBUG_NNUE_UPDATE

    Board->Accumulator.AccumulatorActual = TRUE;

    return TRUE;
}

I32 OutputLayer(const BoardItem* Board)
{
    I32 Result = OutputBias;

#ifdef USE_NNUE_AVX2
    const __m256i Zero = _mm256_setzero_si256();

    __m256i Sum0 = Zero;
    __m256i Sum1 = Zero;
    __m256i Sum2 = Zero;
    __m256i Sum3 = Zero;

    const __m256i* __restrict Accumulator0 = (__m256i*)Board->Accumulator.Accumulator[Board->CurrentColor]; // STM
    const __m256i* __restrict Accumulator1 = (__m256i*)Board->Accumulator.Accumulator[CHANGE_COLOR(Board->CurrentColor)]; // XSTM

    const __m256i* __restrict Weights0 = (__m256i*)&OutputWeights;
    const __m256i* __restrict Weights1 = (__m256i*)&OutputWeights[HIDDEN_DIMENSION];

    for (int Reg = 0; Reg < NUM_REGS; Reg += 4) {
        // STM

        __m256i Acc00 = _mm256_max_epi16(Zero, _mm256_load_si256(&Accumulator0[Reg + 0])); // ReLU
        __m256i Acc01 = _mm256_max_epi16(Zero, _mm256_load_si256(&Accumulator0[Reg + 1])); // ReLU
        __m256i Acc02 = _mm256_max_epi16(Zero, _mm256_load_si256(&Accumulator0[Reg + 2])); // ReLU
        __m256i Acc03 = _mm256_max_epi16(Zero, _mm256_load_si256(&Accumulator0[Reg + 3])); // ReLU

        Sum0 = _mm256_add_epi32(Sum0, _mm256_madd_epi16(Acc00, _mm256_load_si256(&Weights0[Reg + 0])));
        Sum1 = _mm256_add_epi32(Sum1, _mm256_madd_epi16(Acc01, _mm256_load_si256(&Weights0[Reg + 1])));
        Sum2 = _mm256_add_epi32(Sum2, _mm256_madd_epi16(Acc02, _mm256_load_si256(&Weights0[Reg + 2])));
        Sum3 = _mm256_add_epi32(Sum3, _mm256_madd_epi16(Acc03, _mm256_load_si256(&Weights0[Reg + 3])));

        // XSTM

        __m256i Acc10 = _mm256_max_epi16(Zero, _mm256_load_si256(&Accumulator1[Reg + 0])); // ReLU
        __m256i Acc11 = _mm256_max_epi16(Zero, _mm256_load_si256(&Accumulator1[Reg + 1])); // ReLU
        __m256i Acc12 = _mm256_max_epi16(Zero, _mm256_load_si256(&Accumulator1[Reg + 2])); // ReLU
        __m256i Acc13 = _mm256_max_epi16(Zero, _mm256_load_si256(&Accumulator1[Reg + 3])); // ReLU

        Sum0 = _mm256_add_epi32(Sum0, _mm256_madd_epi16(Acc10, _mm256_load_si256(&Weights1[Reg + 0])));
        Sum1 = _mm256_add_epi32(Sum1, _mm256_madd_epi16(Acc11, _mm256_load_si256(&Weights1[Reg + 1])));
        Sum2 = _mm256_add_epi32(Sum2, _mm256_madd_epi16(Acc12, _mm256_load_si256(&Weights1[Reg + 2])));
        Sum3 = _mm256_add_epi32(Sum3, _mm256_madd_epi16(Acc13, _mm256_load_si256(&Weights1[Reg + 3])));
    }

    __m256i R8 = _mm256_add_epi32(_mm256_add_epi32(Sum0, Sum1), _mm256_add_epi32(Sum2, Sum3));
    __m128i R4 = _mm_add_epi32(_mm256_castsi256_si128(R8), _mm256_extractf128_si256(R8, 1));
    __m128i R2 = _mm_add_epi32(R4, _mm_srli_si128(R4, 8));
    __m128i R1 = _mm_add_epi32(R2, _mm_srli_si128(R2, 4));

    Result += _mm_cvtsi128_si32(R1);
#else
    const I16* __restrict Accumulator0 = Board->Accumulator.Accumulator[Board->CurrentColor]; // STM
    const I16* __restrict Accumulator1 = Board->Accumulator.Accumulator[CHANGE_COLOR(Board->CurrentColor)]; // XSTM

    for (int Index = 0; Index < HIDDEN_DIMENSION; Index++) {
        I16 Acc0 = MAX(0, Accumulator0[Index]); // ReLU
        I16 Acc1 = MAX(0, Accumulator1[Index]); // ReLU

        Result += Acc0 * OutputWeights[Index] + Acc1 * OutputWeights[HIDDEN_DIMENSION + Index];
    }
#endif // USE_NNUE_AVX2

    return Result / (QUANTIZATION_PRECISION_IN * QUANTIZATION_PRECISION_OUT);
}

int Evaluate(BoardItem* Board, SearchDataItem* SearchData)
{
#ifdef USE_STATISTIC
    if (SearchData != NULL) {
        SearchData->EvaluateCount++;
    }
#endif // USE_STATISTIC

    if (!Board->Accumulator.AccumulatorActual) {
        if (!UpdateAccumulator(Board)) {
            InitAccumulator(Board);
        }
    }

    return (int)OutputLayer(Board);
}