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
//#define NNUE_FILE_HASH            0x00003BA7AF1FE396
#define NNUE_FILE_SIZE              789520

#define INPUT_DIMENSION             768
#define HIDDEN_DIMENSION            256
#define OUTPUT_DIMENSION            1

#define QUANTIZATION_PRECISION_IN   64
#define QUANTIZATION_PRECISION_OUT  512

#ifdef USE_NNUE_AVX2
#define NUM_REGS                    (HIDDEN_DIMENSION * sizeof(I16) / sizeof(__m256i)) // 16
#endif // USE_NNUE_AVX2

_declspec(align(64)) I16 InputWeights[INPUT_DIMENSION * HIDDEN_DIMENSION];  // 768 x 256 = 196608
_declspec(align(64)) I16 InputBiases[HIDDEN_DIMENSION];                     // 256
_declspec(align(64)) I16 OutputWeights[HIDDEN_DIMENSION * 2];               // 256 x 2 = 512
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

BOOL LoadNetwork(const char* NnueFileName)
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

    fopen_s(&File, NnueFileName, "rb");

    if (File == NULL) { // File open error
        printf("File '%s' open error!\n", NnueFileName);

        return FALSE;
    }

    // File magic

    fread(&FileMagic, 4, 1, File);

//    printf("FileMagic = %d\n", FileMagic);

    if (FileMagic != NNUE_FILE_MAGIC) { // File format error
        printf("File '%s' format error!\n", NnueFileName);

        fclose(File);

        return FALSE;
    }

    // File hash

    fread(&FileHash, sizeof(U64), 1, File);

//    printf("FileHash = 0x%016llX\n", FileHash);
/*
    if (FileHash != NNUE_FILE_HASH) { // File format error
        printf("File '%s' format error!\n", NnueFileName);

        fclose(File);

        return FALSE;
    }
*/
    // Feature weights

#ifdef PRINT_MIN_MAX_VALUES
    MinValue = FLT_MAX;
    MaxValue = -FLT_MAX;
#endif // PRINT_MIN_MAX_VALUES

    for (int Index = 0; Index < INPUT_DIMENSION * HIDDEN_DIMENSION; ++Index) { // 768 x 256 = 196608
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

    for (int Index = 0; Index < HIDDEN_DIMENSION; ++Index) { // 256
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

    for (int Index = 0; Index < HIDDEN_DIMENSION * 2; ++Index) { // 256 x 2 = 512
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

        return FALSE;
    }

    // Set the pointer to the ending of the file

    fseek(File, 0, SEEK_END);

    // Get the current position of the file pointer

    FilePos = ftell(File);

//    printf("File position = %llu\n", FilePos);

    if (FilePos != NNUE_FILE_SIZE) { // File format error
        printf("File '%s' format error!\n", NnueFileName);

        fclose(File);

        return FALSE;
    }

    fclose(File);

    printf("Load network...DONE (%s; 0x%012llx)\n", NnueFileName, FileHash);

    return TRUE;
}

int CalculateWeightIndex(const int Perspective, const int Square, const int PieceWithColor)
{
    int PieceIndex;
    int WeightIndex;

    if (Perspective == WHITE) {
        PieceIndex = PIECE_TYPE(PieceWithColor) + 6 * PIECE_COLOR(PieceWithColor);

        WeightIndex = (PieceIndex << 6) + Square;
    }
    else { // BLACK
        PieceIndex = PIECE_TYPE(PieceWithColor) + 6 * CHANGE_COLOR(PIECE_COLOR(PieceWithColor));

        WeightIndex = (PieceIndex << 6) + (Square ^ 56);
    }

#ifdef PRINT_WEIGHT_INDEX
    printf("Perspective = %d Square = %d Piece = %d Color = %d PieceIndex = %d WeightIndex = %d\n", Perspective, Square, PIECE_TYPE(PieceWithColor), PIECE_COLOR(PieceWithColor), PieceIndex, WeightIndex);
#endif // PRINT_WEIGHT_INDEX

    return WeightIndex;
}

void RefreshAccumulator(BoardItem* Board)
{
    I16 (*Accumulation)[2][HIDDEN_DIMENSION] = &Board->Accumulator.Accumulation;

    for (int Perspective = 0; Perspective < 2; ++Perspective) { // White/Black
        memcpy((*Accumulation)[Perspective], InputBiases, sizeof(InputBiases));

        U64 Pieces = (Board->BB_WhitePieces | Board->BB_BlackPieces);

        while (Pieces) {
            int Square = LSB(Pieces);

            int PieceWithColor = Board->Pieces[Square];

            int WeightIndex = CalculateWeightIndex(Perspective, Square, PieceWithColor);

#ifdef USE_NNUE_AVX2
            __m256i* AccumulatorTile = (__m256i*)(*Accumulation)[Perspective];

            __m256i* Column = (__m256i*)&InputWeights[WeightIndex * HIDDEN_DIMENSION];

            for (int Reg = 0; Reg < NUM_REGS; ++Reg) { // 16
                AccumulatorTile[Reg] = _mm256_add_epi16(AccumulatorTile[Reg], Column[Reg]);
            }
#else
            for (int Index = 0; Index < HIDDEN_DIMENSION; ++Index) { // 256
                (*Accumulation)[Perspective][Index] += InputWeights[WeightIndex * HIDDEN_DIMENSION + Index];
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
    I16 (*Accumulation)[2][HIDDEN_DIMENSION] = &Board->Accumulator.Accumulation;

#ifdef USE_NNUE_AVX2
    __m256i* AccumulatorTile = (__m256i*)(*Accumulation)[Perspective];

    __m256i* Column = (__m256i*)&InputWeights[WeightIndex * HIDDEN_DIMENSION];

    for (int Reg = 0; Reg < NUM_REGS; ++Reg) { // 16
        AccumulatorTile[Reg] = _mm256_add_epi16(AccumulatorTile[Reg], Column[Reg]);
    }
#else
    for (int Index = 0; Index < HIDDEN_DIMENSION; ++Index) { // 256
        (*Accumulation)[Perspective][Index] += InputWeights[WeightIndex * HIDDEN_DIMENSION + Index];
    }
#endif // USE_NNUE_AVX2
}

void AccumulatorSub(BoardItem* Board, const int Perspective, const int WeightIndex)
{
    I16 (*Accumulation)[2][HIDDEN_DIMENSION] = &Board->Accumulator.Accumulation;

#ifdef USE_NNUE_AVX2
    __m256i* AccumulatorTile = (__m256i*)(*Accumulation)[Perspective];

    __m256i* Column = (__m256i*)&InputWeights[WeightIndex * HIDDEN_DIMENSION];

    for (int Reg = 0; Reg < NUM_REGS; ++Reg) { // 16
        AccumulatorTile[Reg] = _mm256_sub_epi16(AccumulatorTile[Reg], Column[Reg]);
    }
#else
    for (int Index = 0; Index < HIDDEN_DIMENSION; ++Index) { // 256
        (*Accumulation)[Perspective][Index] -= InputWeights[WeightIndex * HIDDEN_DIMENSION + Index];
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
        return TRUE;
    }

    for (int Perspective = 0; Perspective < 2; ++Perspective) { // White/Black
        // Delete piece (from)

        PieceWithColor = PIECE_CREATE(Info->PieceTypeFrom, CHANGE_COLOR(Board->CurrentColor));

        WeightIndex = CalculateWeightIndex(Perspective, Info->From, PieceWithColor);

        AccumulatorSub(Board, Perspective, WeightIndex);

        // Delete piece (captured)

        if (Info->Type & MOVE_PAWN_PASSANT) {
            PieceWithColor = PIECE_CREATE(PAWN, Board->CurrentColor);

            WeightIndex = CalculateWeightIndex(Perspective, Info->EatPawnSquare, PieceWithColor);

            AccumulatorSub(Board, Perspective, WeightIndex);
        }
        else if (Info->Type & MOVE_CAPTURE) {
            PieceWithColor = PIECE_CREATE(Info->PieceTypeTo, Board->CurrentColor);

            WeightIndex = CalculateWeightIndex(Perspective, Info->To, PieceWithColor);

            AccumulatorSub(Board, Perspective, WeightIndex);
        }

        // Add piece (to)

        if (Info->Type & MOVE_PAWN_PROMOTE) {
            PieceWithColor = PIECE_CREATE(Info->PromotePieceType, CHANGE_COLOR(Board->CurrentColor));

            WeightIndex = CalculateWeightIndex(Perspective, Info->To, PieceWithColor);

            AccumulatorAdd(Board, Perspective, WeightIndex);
        }
        else {
            PieceWithColor = PIECE_CREATE(Info->PieceTypeFrom, CHANGE_COLOR(Board->CurrentColor));

            WeightIndex = CalculateWeightIndex(Perspective, Info->To, PieceWithColor);

            AccumulatorAdd(Board, Perspective, WeightIndex);
        }
    } // for

#ifdef DEBUG_NNUE
    AccumulatorItem Accumulator = Board->Accumulator;

    RefreshAccumulator(Board);

    for (int Perspective = 0; Perspective < 2; ++Perspective) { // White/Black
        for (int Index = 0; Index < HIDDEN_DIMENSION; ++Index) { // 256
            if (Board->Accumulator.Accumulation[Perspective][Index] != Accumulator.Accumulation[Perspective][Index]) {
                printf("-- Accumulator error! Color = %d Piece = %d From = %d To = %d Move type = %d\n", CHANGE_COLOR(Board->CurrentColor), Info->PieceTypeFrom, Info->From, Info->To, Info->Type);

                break; // for (256)
            }
        }
    }
#endif // DEBUG_NNUE

    Board->Accumulator.AccumulationComputed = TRUE;

    return TRUE;
}

#endif // USE_NNUE_UPDATE

I32 OutputLayer(BoardItem* Board)
{
    I32 Result = OutputBias;

#ifdef PRINT_ACCUMULATOR
    for (int Index = 0; Index < HIDDEN_DIMENSION; ++Index) { // 256
        const I16 Acc0 = Board->Accumulator.Accumulation[Board->CurrentColor][Index];
        const I16 Acc1 = Board->Accumulator.Accumulation[CHANGE_COLOR(Board->CurrentColor)][Index];

        printf("Index = %d Acc0 = %d Acc1 = %d\n", Index, Acc0, Acc1);
    }
#endif // PRINT_ACCUMULATOR

#ifdef USE_NNUE_AVX2
    const __m256i ConstZero = _mm256_setzero_si256();

    __m256i Sum0 = ConstZero;
    __m256i Sum1 = ConstZero;

    __m256i* AccumulatorTile0 = (__m256i*)&Board->Accumulator.Accumulation[Board->CurrentColor];
    __m256i* AccumulatorTile1 = (__m256i*)&Board->Accumulator.Accumulation[CHANGE_COLOR(Board->CurrentColor)];

    __m256i* Weights0 = (__m256i*)&OutputWeights;
    __m256i* Weights1 = (__m256i*)&OutputWeights[HIDDEN_DIMENSION];

    for (int Reg = 0; Reg < NUM_REGS; ++Reg) { // 16
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
    I16 (*Accumulation)[2][HIDDEN_DIMENSION] = &Board->Accumulator.Accumulation;

    for (int Index = 0; Index < HIDDEN_DIMENSION; ++Index) { // 256
        const I16 Acc0 = MAX(0, (*Accumulation)[Board->CurrentColor][Index]); // ReLU
        const I16 Acc1 = MAX(0, (*Accumulation)[CHANGE_COLOR(Board->CurrentColor)][Index]); // ReLU

        Result += Acc0 * OutputWeights[Index]; // Offset 0
        Result += Acc1 * OutputWeights[HIDDEN_DIMENSION + Index]; // Offset 256
    }
#endif // USE_NNUE_AVX2

    return Result / (QUANTIZATION_PRECISION_IN * QUANTIZATION_PRECISION_OUT);
}

int Evaluate(BoardItem* Board)
{
    I32 OutputValue;

#ifdef USE_STATISTIC
    ++Board->EvaluateCount;
#endif // USE_STATISTIC

    // Transform: Board -> (256 x 2)

#ifdef USE_NNUE_UPDATE
    if (!Board->Accumulator.AccumulationComputed) {
        if (!UpdateAccumulator(Board)) {
#endif // USE_NNUE_UPDATE
            RefreshAccumulator(Board);
#ifdef USE_NNUE_UPDATE
        }
    }
#endif // USE_NNUE_UPDATE

    // Output: (256 x 2) -> 1

    OutputValue = OutputLayer(Board);

    return OutputValue;
}