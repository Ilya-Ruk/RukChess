// NNUE2.cpp

#include "stdafx.h"

#include "NNUE2.h"

#include "BitBoard.h"
#include "Board.h"
#include "Def.h"
#include "Types.h"
#include "Utils.h"

#define NNUE_FILE_MAGIC             ('B' | 'R' << 8 | 'K' << 16 | 'R' << 24)
//#define NNUE_FILE_HASH            0x00007342FB032855
#define NNUE_FILE_SIZE              ((INPUT_DIMENSION * HIDDEN_DIMENSION + HIDDEN_DIMENSION + HIDDEN_DIMENSION * 2 + 1) * 4 + 12)

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
    FILE* File;

    int FileMagic;
    U64 FileHash;

    float Value;

    int FilePos;

#ifdef PRINT_MIN_MAX_VALUES
    float MinValue;
    float MaxValue;
#endif // PRINT_MIN_MAX_VALUES

    printf("\n");

    printf("Load network...\n");

    NnueFileLoaded = FALSE; // The network may have been loaded earlier

    fopen_s(&File, NnueFileName, "rb");

    if (File == NULL) { // File open error
        printf("File '%s' open error!\n", NnueFileName);

        return;
    }

    // File magic

    fread(&FileMagic, 4, 1, File);

    if (FileMagic != NNUE_FILE_MAGIC) { // File format error
        printf("File '%s' format error!\n", NnueFileName);

        fclose(File);

        return;
    }

    // File hash

    fread(&FileHash, sizeof(U64), 1, File);
/*
    if (FileHash != NNUE_FILE_HASH) { // File format error
        printf("File '%s' format error!\n", NnueFileName);

        fclose(File);

        return;
    }
*/
    // Feature weights

#ifdef PRINT_MIN_MAX_VALUES
    MinValue = FLT_MAX;
    MaxValue = -FLT_MAX;
#endif // PRINT_MIN_MAX_VALUES

    for (int Index = 0; Index < INPUT_DIMENSION * HIDDEN_DIMENSION; ++Index) {
        fread(&Value, sizeof(float), 1, File);

#ifdef PRINT_MIN_MAX_VALUES
        MinValue = MIN(MinValue, Value);
        MaxValue = MAX(MaxValue, Value);
#endif // PRINT_MIN_MAX_VALUES

        InputWeights[Index] = LoadInt16(Value, QUANTIZATION_PRECISION_IN);
    }

#ifdef PRINT_MIN_MAX_VALUES
    printf("Input weights: MinValue = %f MaxValue = %f\n", MinValue, MaxValue);
#endif // PRINT_MIN_MAX_VALUES

    // Hidden biases

#ifdef PRINT_MIN_MAX_VALUES
    MinValue = FLT_MAX;
    MaxValue = -FLT_MAX;
#endif // PRINT_MIN_MAX_VALUES

    for (int Index = 0; Index < HIDDEN_DIMENSION; ++Index) {
        fread(&Value, sizeof(float), 1, File);

#ifdef PRINT_MIN_MAX_VALUES
        MinValue = MIN(MinValue, Value);
        MaxValue = MAX(MaxValue, Value);
#endif // PRINT_MIN_MAX_VALUES

        InputBiases[Index] = LoadInt16(Value, QUANTIZATION_PRECISION_IN);
    }

#ifdef PRINT_MIN_MAX_VALUES
    printf("Input biases: MinValue = %f MaxValue = %f\n", MinValue, MaxValue);
#endif // PRINT_MIN_MAX_VALUES

    // Hidden weights

#ifdef PRINT_MIN_MAX_VALUES
    MinValue = FLT_MAX;
    MaxValue = -FLT_MAX;
#endif // PRINT_MIN_MAX_VALUES

    for (int Index = 0; Index < HIDDEN_DIMENSION * 2; ++Index) {
        fread(&Value, sizeof(float), 1, File);

#ifdef PRINT_MIN_MAX_VALUES
        MinValue = MIN(MinValue, Value);
        MaxValue = MAX(MaxValue, Value);
#endif // PRINT_MIN_MAX_VALUES

        OutputWeights[Index] = LoadInt16(Value, QUANTIZATION_PRECISION_OUT);
    }

#ifdef PRINT_MIN_MAX_VALUES
    printf("Output weights: MinValue = %f MaxValue = %f\n", MinValue, MaxValue);
#endif // PRINT_MIN_MAX_VALUES

    // Output bias

    fread(&Value, sizeof(float), 1, File);

    OutputBias = LoadInt32(Value, QUANTIZATION_PRECISION_IN * QUANTIZATION_PRECISION_OUT);

#ifdef PRINT_MIN_MAX_VALUES
    printf("Output bias: Value = %f\n", Value);
#endif // PRINT_MIN_MAX_VALUES

    // Get the current position of the file pointer

    FilePos = ftell(File);

    if (FilePos != NNUE_FILE_SIZE) { // File format error
        printf("File '%s' format error!\n", NnueFileName);

        fclose(File);

        return;
    }

    // Set the pointer to the ending of the file

    fseek(File, 0, SEEK_END);

    // Get the current position of the file pointer

    FilePos = ftell(File);

    if (FilePos != NNUE_FILE_SIZE) { // File format error
        printf("File '%s' format error!\n", NnueFileName);

        fclose(File);

        return;
    }

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

#ifdef PRINT_WEIGHT_INDEX
    printf("Perspective = %d Square = %d Piece = %d Color = %d PieceIndex = %d WeightIndex = %d\n", Perspective, Square, PIECE_TYPE(PieceWithColor), PIECE_COLOR(PieceWithColor), PieceIndex, WeightIndex);
#endif // PRINT_WEIGHT_INDEX

    return WeightIndex;
}

void AccumulatorAdd(BoardItem* Board, int Square, int PieceWithColor)
{
    for (int Perspective = 0; Perspective < 2; ++Perspective) { // STM/XSTM
        int WeightIndex = CalculateWeightIndex(Perspective, Square, PieceWithColor);

#ifdef USE_NNUE_AVX2
        __m256i* AccumulatorTile = (__m256i*)Board->Accumulator.Accumulator[Perspective];
        __m256i* Weights = (__m256i*)&InputWeights[WeightIndex * HIDDEN_DIMENSION];

        for (int Reg = 0; Reg < NUM_REGS; ++Reg) {
            AccumulatorTile[Reg] = _mm256_add_epi16(AccumulatorTile[Reg], Weights[Reg]);
        }
#else
        for (int Index = 0; Index < HIDDEN_DIMENSION; ++Index) {
            Board->Accumulator.Accumulator[Perspective][Index] += InputWeights[WeightIndex * HIDDEN_DIMENSION + Index];
        }
#endif // USE_NNUE_AVX2
    }
}

void AccumulatorSub(BoardItem* Board, int Square, int PieceWithColor)
{
    for (int Perspective = 0; Perspective < 2; ++Perspective) { // STM/XSTM
        int WeightIndex = CalculateWeightIndex(Perspective, Square, PieceWithColor);

#ifdef USE_NNUE_AVX2
        __m256i* AccumulatorTile = (__m256i*)Board->Accumulator.Accumulator[Perspective];
        __m256i* Weights = (__m256i*)&InputWeights[WeightIndex * HIDDEN_DIMENSION];

        for (int Reg = 0; Reg < NUM_REGS; ++Reg) {
            AccumulatorTile[Reg] = _mm256_sub_epi16(AccumulatorTile[Reg], Weights[Reg]);
        }
#else
        for (int Index = 0; Index < HIDDEN_DIMENSION; ++Index) {
            Board->Accumulator.Accumulator[Perspective][Index] -= InputWeights[WeightIndex * HIDDEN_DIMENSION + Index];
        }
#endif // USE_NNUE_AVX2
    }
}

void InitAccumulator(BoardItem* Board)
{
    U64 Pieces;

    int Square;
    int PieceWithColor;

    memcpy(Board->Accumulator.Accumulator[STM], InputBiases, sizeof(Board->Accumulator.Accumulator[STM]));
    memcpy(Board->Accumulator.Accumulator[XSTM], InputBiases, sizeof(Board->Accumulator.Accumulator[XSTM]));

    Pieces = (Board->BB_WhitePieces | Board->BB_BlackPieces);

    while (Pieces) {
        Square = LSB(Pieces);
        PieceWithColor = Board->Pieces[Square];

        AccumulatorAdd(Board, Square, PieceWithColor);

        Pieces &= Pieces - 1;
    }
}

I32 OutputLayer(BoardItem* Board)
{
    I32 Result = OutputBias;

#ifdef PRINT_ACCUMULATOR
    for (int Index = 0; Index < HIDDEN_DIMENSION; ++Index) {
        const I16 Acc0 = Board->Accumulator.Accumulator[Board->CurrentColor][Index];
        const I16 Acc1 = Board->Accumulator.Accumulator[CHANGE_COLOR(Board->CurrentColor)][Index];

        printf("Index = %d Acc0 = %d Acc1 = %d\n", Index, Acc0, Acc1);
    }
#endif // PRINT_ACCUMULATOR

#ifdef USE_NNUE_AVX2
    const __m256i ConstZero = _mm256_setzero_si256();

#ifdef USE_CRELU
    const __m256i ConstMax = _mm256_set1_epi16(QUANTIZATION_PRECISION_IN);
#endif // USE_CRELU

    __m256i Sum0 = ConstZero;
    __m256i Sum1 = ConstZero;

    __m256i* AccumulatorTile0 = (__m256i*)Board->Accumulator.Accumulator[Board->CurrentColor];
    __m256i* AccumulatorTile1 = (__m256i*)Board->Accumulator.Accumulator[CHANGE_COLOR(Board->CurrentColor)];

    __m256i* Weights0 = (__m256i*)&OutputWeights;
    __m256i* Weights1 = (__m256i*)&OutputWeights[HIDDEN_DIMENSION];

    for (int Reg = 0; Reg < NUM_REGS; ++Reg) {
#ifdef USE_CRELU
        const __m256i Acc0 = _mm256_max_epi16(ConstZero, _mm256_min_epi16(ConstMax, AccumulatorTile0[Reg])); // CReLU
        const __m256i Acc1 = _mm256_max_epi16(ConstZero, _mm256_min_epi16(ConstMax, AccumulatorTile1[Reg])); // CReLU
#else
        const __m256i Acc0 = _mm256_max_epi16(ConstZero, AccumulatorTile0[Reg]); // ReLU
        const __m256i Acc1 = _mm256_max_epi16(ConstZero, AccumulatorTile1[Reg]); // ReLU
#endif // USE_CRELU

        Sum0 = _mm256_add_epi32(Sum0, _mm256_madd_epi16(Acc0, Weights0[Reg]));
        Sum1 = _mm256_add_epi32(Sum1, _mm256_madd_epi16(Acc1, Weights1[Reg]));
    }

    const __m256i R8 = _mm256_add_epi32(Sum0, Sum1);
    const __m128i R4 = _mm_add_epi32(_mm256_castsi256_si128(R8), _mm256_extractf128_si256(R8, 1));
    const __m128i R2 = _mm_add_epi32(R4, _mm_srli_si128(R4, 8));
    const __m128i R1 = _mm_add_epi32(R2, _mm_srli_si128(R2, 4));

    Result += _mm_cvtsi128_si32(R1);
#else
    for (int Index = 0; Index < HIDDEN_DIMENSION; ++Index) {
#ifdef USE_CRELU
        const I16 Acc0 = MAX(0, MIN(QUANTIZATION_PRECISION_IN, Board->Accumulator.Accumulator[Board->CurrentColor][Index])); // CReLU
        const I16 Acc1 = MAX(0, MIN(QUANTIZATION_PRECISION_IN, Board->Accumulator.Accumulator[CHANGE_COLOR(Board->CurrentColor)][Index])); // CReLU
#else
        const I16 Acc0 = MAX(0, Board->Accumulator.Accumulator[Board->CurrentColor][Index]); // ReLU
        const I16 Acc1 = MAX(0, Board->Accumulator.Accumulator[CHANGE_COLOR(Board->CurrentColor)][Index]); // ReLU
#endif // USE_CRELU

        Result += Acc0 * OutputWeights[Index];
        Result += Acc1 * OutputWeights[HIDDEN_DIMENSION + Index];
    }
#endif // USE_NNUE_AVX2

    return (Result * SCALE) / (QUANTIZATION_PRECISION_IN * QUANTIZATION_PRECISION_OUT);
}

int Evaluate(BoardItem* Board)
{
#ifdef USE_STATISTIC
    ++Board->EvaluateCount;
#endif // USE_STATISTIC

    return (int)OutputLayer(Board);
}