// NNUE2.cpp

#include "stdafx.h"

#include "NNUE2.h"

#include "BitBoard.h"
#include "Board.h"
#include "Def.h"
#include "Types.h"
#include "Utils.h"

#ifdef NNUE_EVALUATION_FUNCTION_2

/*
    https://github.com/Ilya-Ruk/RukChessNets
*/
#define NNUE_FILE_NAME                  "rukchess.nnue"
#define NNUE_FILE_MAGIC                 ('B' | 'R' << 8 | 'K' << 16 | 'R' << 24)
//#define NNUE_FILE_HASH                0x0000755B16A94877
#define NNUE_FILE_SIZE                  820368

#define FEATURE_DIMENSION               768
#define HIDDEN_DIMENSION_1              256
#define HIDDEN_DIMENSION_2              16
#define OUTPUT_DIMENSION                1

#define QUANTIZATION_PRECISION_INPUT    128
#define QUANTIZATION_PRECISION_HIDDEN   64
#define QUANTIZATION_PRECISION_OUTPUT   64

#define HIDDEN_SHIFT                    6 // 2 ^ 6 = 64

#ifdef USE_NNUE_AVX2
#define NUM_REGS                        (HIDDEN_DIMENSION_1 * sizeof(I16) / sizeof(__m256i)) // 32
#endif // USE_NNUE_AVX2

_declspec(align(64)) I16 InputWeights[FEATURE_DIMENSION * HIDDEN_DIMENSION_1];          // 768 x 256 = 196608
_declspec(align(64)) I16 InputBiases[HIDDEN_DIMENSION_1];                               // 256

_declspec(align(64)) I16 HiddenWeights[HIDDEN_DIMENSION_1 * HIDDEN_DIMENSION_2 * 2];    // 256 x 16 x 2 = 8192
_declspec(align(64)) I32 HiddenBiases[HIDDEN_DIMENSION_2];                              // 16

_declspec(align(64)) I16 OutputWeights[HIDDEN_DIMENSION_2];                             // 16
I32 OutputBias;                                                                         // 1

I16 LoadWeight16(const float Value, const int Precision)
{
    return (I16)roundf(Value * (float)Precision);
}

I32 LoadWeight32(const float Value, const int Precision)
{
    return (I32)roundf(Value * (float)Precision);
}

void ReadNetwork(void)
{
    FILE* File;

    int FileMagic;
    U64 FileHash;

    float Value;

    fpos_t FilePos;

#ifdef PRINT_MIN_MAX_VALUES
    float MinValue;
    float MaxValue;
#endif // PRINT_MIN_MAX_VALUES

    printf("\n");

    printf("Load network...\n");

    fopen_s(&File, NNUE_FILE_NAME, "rb");

    if (File == NULL) { // File open error
        printf("File '%s' open error!\n", NNUE_FILE_NAME);

        Sleep(3000);

        exit(0);
    }

    // File magic

    fread(&FileMagic, 4, 1, File);

//    printf("FileMagic = %d\n", FileMagic);

    if (FileMagic != NNUE_FILE_MAGIC) { // File format error
        printf("File '%s' format error!\n", NNUE_FILE_NAME);

        Sleep(3000);

        exit(0);
    }

    // File hash

    fread(&FileHash, sizeof(U64), 1, File);

//    printf("FileHash = 0x%016llX\n", FileHash);
/*
    if (FileHash != NNUE_FILE_HASH) { // File format error
        printf("File '%s' format error!\n", NNUE_FILE_NAME);

        Sleep(3000);

        exit(0);
    }
*/
    // Feature weights

#ifdef PRINT_MIN_MAX_VALUES
    MinValue = FLT_MAX;
    MaxValue = FLT_MIN;
#endif // PRINT_MIN_MAX_VALUES

    for (int Index = 0; Index < FEATURE_DIMENSION * HIDDEN_DIMENSION_1; ++Index) { // 768 x 256 = 196608
        fread(&Value, sizeof(float), 1, File);

#ifdef PRINT_MIN_MAX_VALUES
        if (Value < MinValue) {
            MinValue = Value;
        }

        if (Value > MaxValue) {
            MaxValue = Value;
        }
#endif // PRINT_MIN_MAX_VALUES

        InputWeights[Index] = LoadWeight16(Value, QUANTIZATION_PRECISION_INPUT);
    }

#ifdef PRINT_MIN_MAX_VALUES
    printf("Input weights: MinValue = %f MaxValue = %f\n", MinValue, MaxValue);
#endif // PRINT_MIN_MAX_VALUES

    // Hidden biases 1

#ifdef PRINT_MIN_MAX_VALUES
    MinValue = FLT_MAX;
    MaxValue = FLT_MIN;
#endif // PRINT_MIN_MAX_VALUES

    for (int Index = 0; Index < HIDDEN_DIMENSION_1; ++Index) { // 256
        fread(&Value, sizeof(float), 1, File);

#ifdef PRINT_MIN_MAX_VALUES
        if (Value < MinValue) {
            MinValue = Value;
        }

        if (Value > MaxValue) {
            MaxValue = Value;
        }
#endif // PRINT_MIN_MAX_VALUES

        InputBiases[Index] = LoadWeight16(Value, QUANTIZATION_PRECISION_INPUT);
    }

#ifdef PRINT_MIN_MAX_VALUES
    printf("Input biases: MinValue = %f MaxValue = %f\n", MinValue, MaxValue);
#endif // PRINT_MIN_MAX_VALUES

    // Hidden weights 1

#ifdef PRINT_MIN_MAX_VALUES
    MinValue = FLT_MAX;
    MaxValue = FLT_MIN;
#endif // PRINT_MIN_MAX_VALUES

    for (int Index = 0; Index < HIDDEN_DIMENSION_1 * HIDDEN_DIMENSION_2 * 2; ++Index) { // 256 x 16 x 2 = 8192
        fread(&Value, sizeof(float), 1, File);

#ifdef PRINT_MIN_MAX_VALUES
        if (Value < MinValue) {
            MinValue = Value;
        }

        if (Value > MaxValue) {
            MaxValue = Value;
        }
#endif // PRINT_MIN_MAX_VALUES

        HiddenWeights[Index] = LoadWeight16(Value, QUANTIZATION_PRECISION_HIDDEN);
    }

#ifdef PRINT_MIN_MAX_VALUES
    printf("Hidden weights: MinValue = %f MaxValue = %f\n", MinValue, MaxValue);
#endif // PRINT_MIN_MAX_VALUES

    // Hidden biases 2

#ifdef PRINT_MIN_MAX_VALUES
    MinValue = FLT_MAX;
    MaxValue = FLT_MIN;
#endif // PRINT_MIN_MAX_VALUES

    for (int Index = 0; Index < HIDDEN_DIMENSION_2; ++Index) { // 16
        fread(&Value, sizeof(float), 1, File);

#ifdef PRINT_MIN_MAX_VALUES
        if (Value < MinValue) {
            MinValue = Value;
        }

        if (Value > MaxValue) {
            MaxValue = Value;
        }
#endif // PRINT_MIN_MAX_VALUES

        HiddenBiases[Index] = LoadWeight32(Value, QUANTIZATION_PRECISION_HIDDEN * QUANTIZATION_PRECISION_INPUT);
    }

#ifdef PRINT_MIN_MAX_VALUES
    printf("Hidden biases: MinValue = %f MaxValue = %f\n", MinValue, MaxValue);
#endif // PRINT_MIN_MAX_VALUES

    // Hidden weights 2

#ifdef PRINT_MIN_MAX_VALUES
    MinValue = FLT_MAX;
    MaxValue = FLT_MIN;
#endif // PRINT_MIN_MAX_VALUES

    for (int Index = 0; Index < HIDDEN_DIMENSION_2; ++Index) { // 16
        fread(&Value, sizeof(float), 1, File);

#ifdef PRINT_MIN_MAX_VALUES
        if (Value < MinValue) {
            MinValue = Value;
        }

        if (Value > MaxValue) {
            MaxValue = Value;
        }
#endif // PRINT_MIN_MAX_VALUES

        OutputWeights[Index] = LoadWeight16(Value, QUANTIZATION_PRECISION_OUTPUT);
    }

#ifdef PRINT_MIN_MAX_VALUES
    printf("Output weights: MinValue = %f MaxValue = %f\n", MinValue, MaxValue);
#endif // PRINT_MIN_MAX_VALUES

    // Output bias

    fread(&Value, sizeof(float), 1, File);

    OutputBias = LoadWeight32(Value, QUANTIZATION_PRECISION_OUTPUT * QUANTIZATION_PRECISION_INPUT);

#ifdef PRINT_MIN_MAX_VALUES
    printf("Output bias: Value = %f\n", Value);
#endif // PRINT_MIN_MAX_VALUES

    fgetpos(File, &FilePos);

//    printf("File position = %llu\n", FilePos);

    if (FilePos != NNUE_FILE_SIZE) { // File format error
        printf("File '%s' format error!\n", NNUE_FILE_NAME);

        Sleep(3000);

        exit(0);
    }

    fclose(File);

    printf("Load network...DONE (%s; hash = 0x%016llX)\n", NNUE_FILE_NAME, FileHash);
}

int CalculateWeightIndex(const int Perspective, const int Square, const int PieceWithColor)
{
    int PieceIndex;
    int WeightIndex;

    if (Perspective == WHITE) {
        PieceIndex = PIECE(PieceWithColor) + 6 * COLOR(PieceWithColor);

        WeightIndex = (PieceIndex << 6) + Square;
    }
    else { // BLACK
        PieceIndex = PIECE(PieceWithColor) + 6 * CHANGE_COLOR(COLOR(PieceWithColor));

        WeightIndex = (PieceIndex << 6) + (Square ^ 56);
    }

#ifdef PRINT_WEIGHT_INDEX
    printf("Perspective = %d Square = %d Piece = %d Color = %d PieceIndex = %d WeightIndex = %d\n", Perspective, Square, PIECE(PieceWithColor), COLOR(PieceWithColor), PieceIndex, WeightIndex);
#endif // PRINT_WEIGHT_INDEX

    return WeightIndex;
}

void RefreshAccumulator(BoardItem* Board)
{
    I16 (*Accumulation)[2][HIDDEN_DIMENSION_1] = &Board->Accumulator.Accumulation;

    for (int Perspective = 0; Perspective < 2; ++Perspective) { // White/Black
        memcpy((*Accumulation)[Perspective], InputBiases, sizeof(InputBiases));

        U64 Pieces = (Board->BB_WhitePieces | Board->BB_BlackPieces);

        while (Pieces) {
            int Square = LSB(Pieces);

            int PieceWithColor = Board->Pieces[Square];

            int WeightIndex = CalculateWeightIndex(Perspective, Square, PieceWithColor);

#ifdef USE_NNUE_AVX2
            __m256i* AccumulatorTile = (__m256i*)(*Accumulation)[Perspective];

            __m256i* Column = (__m256i*)&InputWeights[WeightIndex * HIDDEN_DIMENSION_1];

            for (int Reg = 0; Reg < NUM_REGS; ++Reg) { // 32
                AccumulatorTile[Reg] = _mm256_add_epi16(AccumulatorTile[Reg], Column[Reg]);
            }
#else
            for (int Index = 0; Index < HIDDEN_DIMENSION_1; ++Index) { // 512
                (*Accumulation)[Perspective][Index] += InputWeights[WeightIndex * HIDDEN_DIMENSION_1 + Index];
            }
#endif // USE_NNUE_AVX2

            Pieces &= Pieces - 1;
        }
    }

#ifdef USE_NNUE_UPDATE
    Board->Accumulator.AccumulationComputed = TRUE;
#endif // USE_NNUE_UPDATE
}

#ifdef USE_NNUE_UPDATE

void AccumulatorAdd(BoardItem* Board, const int Perspective, const int WeightIndex)
{
    I16 (*Accumulation)[2][HIDDEN_DIMENSION_1] = &Board->Accumulator.Accumulation;

#ifdef USE_NNUE_AVX2
    __m256i* AccumulatorTile = (__m256i*)(*Accumulation)[Perspective];

    __m256i* Column = (__m256i*)&InputWeights[WeightIndex * HIDDEN_DIMENSION_1];

    for (int Reg = 0; Reg < NUM_REGS; ++Reg) { // 32
        AccumulatorTile[Reg] = _mm256_add_epi16(AccumulatorTile[Reg], Column[Reg]);
    }
#else
    for (int Index = 0; Index < HIDDEN_DIMENSION_1; ++Index) { // 512
        (*Accumulation)[Perspective][Index] += InputWeights[WeightIndex * HIDDEN_DIMENSION_1 + Index];
    }
#endif // USE_NNUE_AVX2
}

void AccumulatorSub(BoardItem* Board, const int Perspective, const int WeightIndex)
{
    I16 (*Accumulation)[2][HIDDEN_DIMENSION_1] = &Board->Accumulator.Accumulation;

#ifdef USE_NNUE_AVX2
    __m256i* AccumulatorTile = (__m256i*)(*Accumulation)[Perspective];

    __m256i* Column = (__m256i*)&InputWeights[WeightIndex * HIDDEN_DIMENSION_1];

    for (int Reg = 0; Reg < NUM_REGS; ++Reg) { // 32
        AccumulatorTile[Reg] = _mm256_sub_epi16(AccumulatorTile[Reg], Column[Reg]);
    }
#else
    for (int Index = 0; Index < HIDDEN_DIMENSION_1; ++Index) { // 512
        (*Accumulation)[Perspective][Index] -= InputWeights[WeightIndex * HIDDEN_DIMENSION_1 + Index];
    }
#endif // USE_NNUE_AVX2
}

BOOL UpdateAccumulator(BoardItem* Board)
{
    HistoryItem* Info;

    int PieceWithColor;

    int WeightIndex;

    if (Board->HalfMoveNumber == 0) {
        return FALSE;
    }

    Info = &Board->MoveTable[Board->HalfMoveNumber - 1]; // Prev. move info

    if (!Info->Accumulator.AccumulationComputed) {
        return FALSE;
    }

    if (Info->Type & (MOVE_CASTLE_KING | MOVE_CASTLE_QUEEN)) {
        return FALSE;
    }

    if (Info->Type & MOVE_NULL) {
        Board->Accumulator.AccumulationComputed = TRUE;

        return TRUE;
    }

    for (int Perspective = 0; Perspective < 2; ++Perspective) { // White/Black
        // Delete piece (from)

        PieceWithColor = PIECE_AND_COLOR(Info->PieceFrom, CHANGE_COLOR(Board->CurrentColor));

        WeightIndex = CalculateWeightIndex(Perspective, Info->From, PieceWithColor);

        AccumulatorSub(Board, Perspective, WeightIndex);

        // Delete piece (captured)

        if (Info->Type & MOVE_PAWN_PASSANT) {
            PieceWithColor = PIECE_AND_COLOR(PAWN, Board->CurrentColor);

            WeightIndex = CalculateWeightIndex(Perspective, Info->EatPawnSquare, PieceWithColor);

            AccumulatorSub(Board, Perspective, WeightIndex);
        }
        else if (Info->Type & MOVE_CAPTURE) {
            PieceWithColor = PIECE_AND_COLOR(Info->PieceTo, Board->CurrentColor);

            WeightIndex = CalculateWeightIndex(Perspective, Info->To, PieceWithColor);

            AccumulatorSub(Board, Perspective, WeightIndex);
        }

        // Add piece (to)

        if (Info->Type & MOVE_PAWN_PROMOTE) {
            PieceWithColor = PIECE_AND_COLOR(Info->PromotePiece, CHANGE_COLOR(Board->CurrentColor));

            WeightIndex = CalculateWeightIndex(Perspective, Info->To, PieceWithColor);

            AccumulatorAdd(Board, Perspective, WeightIndex);
        }
        else {
            PieceWithColor = PIECE_AND_COLOR(Info->PieceFrom, CHANGE_COLOR(Board->CurrentColor));

            WeightIndex = CalculateWeightIndex(Perspective, Info->To, PieceWithColor);

            AccumulatorAdd(Board, Perspective, WeightIndex);
        }
    } // for

#ifdef DEBUG_NNUE_2
    AccumulatorItem Accumulator = Board->Accumulator;

    RefreshAccumulator(Board);

    for (int Perspective = 0; Perspective < 2; ++Perspective) { // White/Black
        for (int Index = 0; Index < HIDDEN_DIMENSION; ++Index) { // 512
            if (Board->Accumulator.Accumulation[Perspective][Index] != Accumulator.Accumulation[Perspective][Index]) {
                printf("-- Accumulator error! Color = %d Piece = %d From = %d To = %d Move type = %d\n", CHANGE_COLOR(Board->CurrentColor), Info->PieceFrom, Info->From, Info->To, Info->Type);

                break; // for (512)
            }
        }
    }
#endif // DEBUG_NNUE_2

    Board->Accumulator.AccumulationComputed = TRUE;

    return TRUE;
}

#endif // USE_NNUE_UPDATE

void HiddenLayer(BoardItem* Board, I16 HiddenValues16[])
{
#ifdef PRINT_ACCUMULATOR
    for (int Index = 0; Index < HIDDEN_DIMENSION_1; ++Index) { // 512
        const I16 Acc0 = Board->Accumulator.Accumulation[Board->CurrentColor][Index];
        const I16 Acc1 = Board->Accumulator.Accumulation[CHANGE_COLOR(Board->CurrentColor)][Index];

        printf("Index = %d Acc0 = %d Acc1 = %d\n", Index, Acc0, Acc1);
    }
#endif // PRINT_ACCUMULATOR

#ifdef USE_NNUE_AVX2
    const __m256i ConstZero = _mm256_setzero_si256();
    const __m256i ConstMax = _mm256_set1_epi16(QUANTIZATION_PRECISION_INPUT);

    __m256i* AccumulatorTile0 = (__m256i*)&Board->Accumulator.Accumulation[Board->CurrentColor];
    __m256i* AccumulatorTile1 = (__m256i*)&Board->Accumulator.Accumulation[CHANGE_COLOR(Board->CurrentColor)];

    for (int Index = 0; Index < HIDDEN_DIMENSION_2; ++Index) { // 32
        __m256i Sum0 = ConstZero;
        __m256i Sum1 = ConstZero;

        __m256i* Weights0 = (__m256i*)&HiddenWeights[Index * 2 * HIDDEN_DIMENSION_1];
        __m256i* Weights1 = (__m256i*)&HiddenWeights[Index * 2 * HIDDEN_DIMENSION_1 + HIDDEN_DIMENSION_1];

        for (int Reg = 0; Reg < NUM_REGS; ++Reg) { // 32
            const __m256i Acc0 = _mm256_max_epi16(ConstZero, _mm256_min_epi16(ConstMax, AccumulatorTile0[Reg])); // CReLU
            const __m256i Acc1 = _mm256_max_epi16(ConstZero, _mm256_min_epi16(ConstMax, AccumulatorTile1[Reg])); // CReLU

            Sum0 = _mm256_add_epi32(Sum0, _mm256_madd_epi16(Acc0, Weights0[Reg]));
            Sum1 = _mm256_add_epi32(Sum1, _mm256_madd_epi16(Acc1, Weights1[Reg]));
        }

        const __m256i R8 = _mm256_add_epi32(Sum0, Sum1);
        const __m128i R4 = _mm_add_epi32(_mm256_castsi256_si128(R8), _mm256_extractf128_si256(R8, 1));
        const __m128i R2 = _mm_add_epi32(R4, _mm_srli_si128(R4, 8));
        const __m128i R1 = _mm_add_epi32(R2, _mm_srli_si128(R2, 4));

        HiddenValues16[Index] = (I16)((_mm_cvtsi128_si32(R1) + HiddenBiases[Index]) >> HIDDEN_SHIFT);
    }
#else
    I16 (*Accumulation)[2][HIDDEN_DIMENSION_1] = &Board->Accumulator.Accumulation;

    for (int Index1 = 0; Index1 < HIDDEN_DIMENSION_2; ++Index1) { // 32
        I32 Result = HiddenBiases[Index1];

        for (int Index2 = 0; Index2 < HIDDEN_DIMENSION_1; ++Index2) { // 512
            const I16 Acc0 = MAX(0, MIN(QUANTIZATION_PRECISION_INPUT, (*Accumulation)[Board->CurrentColor][Index2])); // CReLU
            const I16 Acc1 = MAX(0, MIN(QUANTIZATION_PRECISION_INPUT, (*Accumulation)[CHANGE_COLOR(Board->CurrentColor)][Index2])); // CReLU

            Result += Acc0 * HiddenWeights[Index1 * 2 * HIDDEN_DIMENSION_1 + Index2]; // Offset 0
            Result += Acc1 * HiddenWeights[Index1 * 2 * HIDDEN_DIMENSION_1 + Index2 + HIDDEN_DIMENSION_1]; // Offset 512
        }

        HiddenValues16[Index1] = (I16)(Result >> HIDDEN_SHIFT);
    }
#endif // USE_NNUE_AVX2
}

I32 OutputLayer(BoardItem* Board, I16 HiddenValues16[])
{
    I32 Result = OutputBias;

#ifdef USE_NNUE_AVX2
    const __m256i ConstZero = _mm256_setzero_si256();
    const __m256i ConstMax = _mm256_set1_epi16(QUANTIZATION_PRECISION_INPUT);

    __m256i* AccumulatorTile0 = (__m256i*)&HiddenValues16[0];
//    __m256i* AccumulatorTile1 = (__m256i*)&HiddenValues16[HIDDEN_DIMENSION_2 / 2];

    __m256i* Weights0 = (__m256i*)&OutputWeights[0];
//    __m256i* Weights1 = (__m256i*)&OutputWeights[HIDDEN_DIMENSION_2 / 2];

    const __m256i Acc0 = _mm256_max_epi16(ConstZero, _mm256_min_epi16(ConstMax, AccumulatorTile0[0])); // CReLU
//    const __m256i Acc1 = _mm256_max_epi16(ConstZero, _mm256_min_epi16(ConstMax, AccumulatorTile1[0])); // ReLU

    __m256i Sum0 = _mm256_madd_epi16(Acc0, Weights0[0]);
//    __m256i Sum1 = _mm256_madd_epi16(Acc1, Weights1[0]);

    const __m256i R8 = _mm256_add_epi32(Sum0, ConstZero);
    const __m128i R4 = _mm_add_epi32(_mm256_castsi256_si128(R8), _mm256_extractf128_si256(R8, 1));
    const __m128i R2 = _mm_add_epi32(R4, _mm_srli_si128(R4, 8));
    const __m128i R1 = _mm_add_epi32(R2, _mm_srli_si128(R2, 4));

    Result += _mm_cvtsi128_si32(R1);
#else
    for (int Index = 0; Index < HIDDEN_DIMENSION_2; ++Index) { // 32
        const I16 Value = MAX(0, MIN(QUANTIZATION_PRECISION_INPUT, HiddenValues16[Index])); // CReLU

        Result += (I32)Value * OutputWeights[Index];
    }
#endif // USE_NNUE_AVX2

    return Result / QUANTIZATION_PRECISION_INPUT / QUANTIZATION_PRECISION_OUTPUT;
}

int NetworkEvaluate(BoardItem* Board)
{
    I16 HiddenValues16[HIDDEN_DIMENSION_2];

    I32 OutputValue;

    // Transform: Board -> (512 x 2)

    if (!Board->Accumulator.AccumulationComputed) {
#ifdef USE_NNUE_UPDATE
        if (!UpdateAccumulator(Board)) {
#endif // USE_NNUE_UPDATE
            RefreshAccumulator(Board);
#ifdef USE_NNUE_UPDATE
        }
#endif // USE_NNUE_UPDATE
    }

    // Hidden layer: (512 x 2) -> 32

    HiddenLayer(Board, HiddenValues16);

    // Output: 32 -> 1

    OutputValue = OutputLayer(Board, HiddenValues16);

    return OutputValue;
}

#endif // NNUE_EVALUATION_FUNCTION_2