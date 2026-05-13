// NNUE2.cpp

#include "stdafx.h"

#include "NNUE2.h"

#include "BitBoard.h"
#include "Board.h"
#include "Def.h"
#include "Game.h"
#include "Types.h"
#include "Utils.h"

#define NNUE_FILE_MAGIC             ('B' | 'R' << 8 | 'K' << 16 | 'R' << 24)
//#define NNUE_FILE_HASH            0X00007342FB032855
#define NNUE_FILE_SIZE              1579024

#define INPUT_DIMENSION             768
#define HIDDEN_DIMENSION            512
#define OUTPUT_DIMENSION            1

#define QUANTIZATION_PRECISION_IN   64
#define QUANTIZATION_PRECISION_OUT  512

#define STM                         0
#define XSTM                        1

#ifdef USE_NNUE_AVX2
#define NUM_REGS                    (HIDDEN_DIMENSION * sizeof(I16) / sizeof(__m256i)) // 32
#endif // USE_NNUE_AVX2

_declspec(align(64)) I16 InputWeights[INPUT_DIMENSION * HIDDEN_DIMENSION];  // 768 x 512 = 393216
_declspec(align(64)) I16 InputBiases[HIDDEN_DIMENSION];                     // 512
_declspec(align(64)) I16 OutputWeights[HIDDEN_DIMENSION * 2];               // 512 x 2 = 1024
I32 OutputBias;                                                             // 1

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

//    printf("FileMagic = %d\n", FileMagic);

    if (FileMagic != NNUE_FILE_MAGIC) { // File format error
        printf("File '%s' format error!\n", NnueFileName);

        fclose(File);

        return;
    }

    // File hash

    fread(&FileHash, sizeof(U64), 1, File);

//    printf("FileHash = 0x%016llX\n", FileHash);
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

    for (int Index = 0; Index < INPUT_DIMENSION * HIDDEN_DIMENSION; ++Index) { // 768 x 512 = 393216
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

    for (int Index = 0; Index < HIDDEN_DIMENSION; ++Index) { // 512
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

    for (int Index = 0; Index < HIDDEN_DIMENSION * 2; ++Index) { // 512 x 2 = 1024
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

//    printf("File position = %llu\n", FilePos);

    if (FilePos != NNUE_FILE_SIZE) { // File format error
        printf("File '%s' format error!\n", NnueFileName);

        fclose(File);

        return;
    }

    // Set the pointer to the ending of the file

    fseek(File, 0, SEEK_END);

    // Get the current position of the file pointer

    FilePos = ftell(File);

//    printf("File position = %llu\n", FilePos);

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
    int WeightIndex;

#ifdef USE_NNUE_AVX2
    __m256i* AccumulatorTile;
    __m256i* Weights;
#endif // USE_NNUE_AVX2

    for (int Perspective = STM; Perspective <= XSTM; ++Perspective) {
        WeightIndex = CalculateWeightIndex(Perspective, Square, PieceWithColor);

#ifdef USE_NNUE_AVX2
        AccumulatorTile = (__m256i*)&Board->Accumulator.Accumulator[Perspective];
        Weights = (__m256i*)&InputWeights[WeightIndex * HIDDEN_DIMENSION];

        for (int Reg = 0; Reg < NUM_REGS; ++Reg) { // 32
            AccumulatorTile[Reg] = _mm256_add_epi16(AccumulatorTile[Reg], Weights[Reg]);
        }
#else
        for (int Index = 0; Index < HIDDEN_DIMENSION; ++Index) { // 512
            Board->Accumulator.Accumulator[Perspective][Index] += InputWeights[WeightIndex * HIDDEN_DIMENSION + Index];
        }
#endif // USE_NNUE_AVX2
    }
}

void AccumulatorSub(BoardItem* Board, int Square, int PieceWithColor)
{
    int WeightIndex;

#ifdef USE_NNUE_AVX2
    __m256i* AccumulatorTile;
    __m256i* Weights;
#endif // USE_NNUE_AVX2

    for (int Perspective = STM; Perspective <= XSTM; ++Perspective) {
        WeightIndex = CalculateWeightIndex(Perspective, Square, PieceWithColor);

#ifdef USE_NNUE_AVX2
        AccumulatorTile = (__m256i*)&Board->Accumulator.Accumulator[Perspective];
        Weights = (__m256i*)&InputWeights[WeightIndex * HIDDEN_DIMENSION];

        for (int Reg = 0; Reg < NUM_REGS; ++Reg) { // 32
            AccumulatorTile[Reg] = _mm256_sub_epi16(AccumulatorTile[Reg], Weights[Reg]);
        }
#else
        for (int Index = 0; Index < HIDDEN_DIMENSION; ++Index) { // 512
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
    for (int Index = 0; Index < HIDDEN_DIMENSION; ++Index) { // 512
        const I16 Acc0 = Board->Accumulator.Accumulator[Board->CurrentColor][Index];
        const I16 Acc1 = Board->Accumulator.Accumulator[CHANGE_COLOR(Board->CurrentColor)][Index];

        printf("Index = %d Acc0 = %d Acc1 = %d\n", Index, Acc0, Acc1);
    }
#endif // PRINT_ACCUMULATOR

#ifdef USE_NNUE_AVX2
    const __m256i ConstZero = _mm256_setzero_si256();

    __m256i Sum0 = ConstZero;
    __m256i Sum1 = ConstZero;

    __m256i* AccumulatorTile0 = (__m256i*)&Board->Accumulator.Accumulator[Board->CurrentColor];
    __m256i* AccumulatorTile1 = (__m256i*)&Board->Accumulator.Accumulator[CHANGE_COLOR(Board->CurrentColor)];

    __m256i* Weights0 = (__m256i*)&OutputWeights;
    __m256i* Weights1 = (__m256i*)&OutputWeights[HIDDEN_DIMENSION];

    for (int Reg = 0; Reg < NUM_REGS; ++Reg) { // 32
        const __m256i Acc0 = _mm256_max_epi16(ConstZero, AccumulatorTile0[Reg]); // ReLU
        const __m256i Acc1 = _mm256_max_epi16(ConstZero, AccumulatorTile1[Reg]); // ReLU

        Sum0 = _mm256_add_epi32(Sum0, _mm256_madd_epi16(Acc0, Weights0[Reg]));
        Sum1 = _mm256_add_epi32(Sum1, _mm256_madd_epi16(Acc1, Weights1[Reg]));
    }

    const __m256i R8 = _mm256_add_epi32(Sum0, Sum1);
    const __m128i R4 = _mm_add_epi32(_mm256_castsi256_si128(R8), _mm256_extractf128_si256(R8, 1));
    const __m128i R2 = _mm_add_epi32(R4, _mm_srli_si128(R4, 8));
    const __m128i R1 = _mm_add_epi32(R2, _mm_srli_si128(R2, 4));

    Result += _mm_cvtsi128_si32(R1);
#else
    for (int Index = 0; Index < HIDDEN_DIMENSION; ++Index) { // 512
        const I16 Acc0 = MAX(0, Board->Accumulator.Accumulator[Board->CurrentColor][Index]); // ReLU
        const I16 Acc1 = MAX(0, Board->Accumulator.Accumulator[CHANGE_COLOR(Board->CurrentColor)][Index]); // ReLU

        Result += Acc0 * OutputWeights[Index]; // Offset 0
        Result += Acc1 * OutputWeights[HIDDEN_DIMENSION + Index]; // Offset 512
    }
#endif // USE_NNUE_AVX2

    return Result / (QUANTIZATION_PRECISION_IN * QUANTIZATION_PRECISION_OUT);
}

int Evaluate(BoardItem* Board)
{
#ifdef USE_STATISTIC
    ++Board->EvaluateCount;
#endif // USE_STATISTIC

    return (int)OutputLayer(Board);
}