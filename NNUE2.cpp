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

    FILE* File;

    fopen_s(&File, NnueFileName, "rb");

    if (File == NULL) { // File open error
        printf("File '%s' open error!\n", NnueFileName);

        return;
    }

    // File magic

    int FileMagic;

    fread(&FileMagic, 4, 1, File);

    if (FileMagic != NNUE_FILE_MAGIC) { // File format error
        printf("File '%s' format error!\n", NnueFileName);

        fclose(File);

        return;
    }

    // File hash (part 1)

    U64 FileHash;

    fread(&FileHash, sizeof(U64), 1, File);

    // Weights and biases

    float Value;

    U64 NnueHash = 0ULL;

    // Feature weights

    for (int Index = 0; Index < INPUT_DIMENSION * HIDDEN_DIMENSION; Index++) {
        fread(&Value, sizeof(float), 1, File);

        NnueHash = NNUE_HASH(NnueHash, (U64)Value);

        InputWeights[Index] = LoadInt16(Value, QUANTIZATION_PRECISION_IN);
    }

    // Hidden biases

    for (int Index = 0; Index < HIDDEN_DIMENSION; Index++) {
        fread(&Value, sizeof(float), 1, File);

        NnueHash = NNUE_HASH(NnueHash, (U64)Value);

        InputBiases[Index] = LoadInt16(Value, QUANTIZATION_PRECISION_IN);
    }

    // Hidden weights

    for (int Index = 0; Index < HIDDEN_DIMENSION * 2; Index++) {
        fread(&Value, sizeof(float), 1, File);

        NnueHash = NNUE_HASH(NnueHash, (U64)Value);

        OutputWeights[Index] = LoadInt16(Value, QUANTIZATION_PRECISION_OUT);
    }

    // Output bias

    fread(&Value, sizeof(float), 1, File);

    NnueHash = NNUE_HASH(NnueHash, (U64)Value);

    OutputBias = LoadInt32(Value, QUANTIZATION_PRECISION_IN * QUANTIZATION_PRECISION_OUT);

    // Get the current position of the file pointer

    int FilePos1 = ftell(File);

    if (FilePos1 != NNUE_FILE_SIZE) { // File format error
        printf("File '%s' format error!\n", NnueFileName);

        fclose(File);

        return;
    }

    // Set the pointer to the ending of the file

    fseek(File, 0, SEEK_END);

    // Get the current position of the file pointer

    int FilePos2 = ftell(File);

    if (FilePos2 != NNUE_FILE_SIZE) { // File format error
        printf("File '%s' format error!\n", NnueFileName);

        fclose(File);

        return;
    }

    // File hash (part 2)

    if (FileHash != NnueHash) { // File format error
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

    return WeightIndex;
}

void AccumulatorAdd(BoardItem* Board, int Square, int PieceWithColor)
{
    for (int Perspective = 0; Perspective < 2; Perspective++) { // STM/XSTM
        int WeightIndex = CalculateWeightIndex(Perspective, Square, PieceWithColor);

#ifdef USE_NNUE_AVX2
        __m256i* AccumulatorTile = (__m256i*)Board->Accumulator.Accumulator[Perspective];
        __m256i* Weights = (__m256i*)&InputWeights[WeightIndex * HIDDEN_DIMENSION];

        for (int Reg = 0; Reg < NUM_REGS; Reg++) {
            AccumulatorTile[Reg] = _mm256_add_epi16(AccumulatorTile[Reg], Weights[Reg]);
        }
#else
        for (int Index = 0; Index < HIDDEN_DIMENSION; Index++) {
            Board->Accumulator.Accumulator[Perspective][Index] += InputWeights[WeightIndex * HIDDEN_DIMENSION + Index];
        }
#endif // USE_NNUE_AVX2
    }
}

void AccumulatorSub(BoardItem* Board, int Square, int PieceWithColor)
{
    for (int Perspective = 0; Perspective < 2; Perspective++) { // STM/XSTM
        int WeightIndex = CalculateWeightIndex(Perspective, Square, PieceWithColor);

#ifdef USE_NNUE_AVX2
        __m256i* AccumulatorTile = (__m256i*)Board->Accumulator.Accumulator[Perspective];
        __m256i* Weights = (__m256i*)&InputWeights[WeightIndex * HIDDEN_DIMENSION];

        for (int Reg = 0; Reg < NUM_REGS; Reg++) {
            AccumulatorTile[Reg] = _mm256_sub_epi16(AccumulatorTile[Reg], Weights[Reg]);
        }
#else
        for (int Index = 0; Index < HIDDEN_DIMENSION; Index++) {
            Board->Accumulator.Accumulator[Perspective][Index] -= InputWeights[WeightIndex * HIDDEN_DIMENSION + Index];
        }
#endif // USE_NNUE_AVX2
    }
}

void InitAccumulator(BoardItem* Board)
{
    memcpy(Board->Accumulator.Accumulator[STM], InputBiases, sizeof(Board->Accumulator.Accumulator[STM]));
    memcpy(Board->Accumulator.Accumulator[XSTM], InputBiases, sizeof(Board->Accumulator.Accumulator[XSTM]));

    U64 Pieces = (Board->BB_WhitePieces | Board->BB_BlackPieces);

    while (Pieces) {
        int Square = LSB(Pieces);

        int PieceWithColor = Board->Pieces[Square];

        AccumulatorAdd(Board, Square, PieceWithColor);

        Pieces &= Pieces - 1ULL;
    }

    Board->Accumulator.AccumulatorActual = TRUE;
}

BOOL UpdateAccumulator(BoardItem* Board)
{
    int Color = Board->CurrentColor;

    for (int MoveIndex = Board->HalfMoveNumber - 1, UpdateMoves = 0; MoveIndex >= 0, UpdateMoves < MAX_UPDATE_MOVES; MoveIndex--, UpdateMoves++) {
        HistoryItem* Info = &Board->MoveTable[MoveIndex];

        Color = CHANGE_COLOR(Color);

        if (!(Info->Type & MOVE_NULL)) {
            if (Color == WHITE) {
                if (Info->Type & MOVE_CASTLE_KING) {
                    AccumulatorSub(Board, SQ_H1, PIECE_CREATE(ROOK, WHITE));
                    AccumulatorAdd(Board, SQ_F1, PIECE_CREATE(ROOK, WHITE));
                }

                if (Info->Type & MOVE_CASTLE_QUEEN) {
                    AccumulatorSub(Board, SQ_A1, PIECE_CREATE(ROOK, WHITE));
                    AccumulatorAdd(Board, SQ_D1, PIECE_CREATE(ROOK, WHITE));
                }

                if (Info->Type & MOVE_PAWN_EN_PASSANT) {
                    AccumulatorSub(Board, Info->CapturePawnSquare, PIECE_CREATE(PAWN, BLACK));
                }
                else if (Info->Type & MOVE_CAPTURE) {
                    AccumulatorSub(Board, Info->To, PIECE_CREATE(Info->PieceTypeTo, BLACK));
                }

                if (Info->Type & MOVE_PAWN_PROMOTE) {
                    AccumulatorAdd(Board, Info->To, PIECE_CREATE(Info->PromotePieceType, WHITE));
                }
                else {
                    AccumulatorAdd(Board, Info->To, PIECE_CREATE(Info->PieceTypeFrom, WHITE));
                }

                AccumulatorSub(Board, Info->From, PIECE_CREATE(Info->PieceTypeFrom, WHITE));
            }
            else { // BLACK
                if (Info->Type & MOVE_CASTLE_KING) {
                    AccumulatorSub(Board, SQ_H8, PIECE_CREATE(ROOK, BLACK));
                    AccumulatorAdd(Board, SQ_F8, PIECE_CREATE(ROOK, BLACK));
                }

                if (Info->Type & MOVE_CASTLE_QUEEN) {
                    AccumulatorSub(Board, SQ_A8, PIECE_CREATE(ROOK, BLACK));
                    AccumulatorAdd(Board, SQ_D8, PIECE_CREATE(ROOK, BLACK));
                }

                if (Info->Type & MOVE_PAWN_EN_PASSANT) {
                    AccumulatorSub(Board, Info->CapturePawnSquare, PIECE_CREATE(PAWN, WHITE));
                }
                else if (Info->Type & MOVE_CAPTURE) {
                    AccumulatorSub(Board, Info->To, PIECE_CREATE(Info->PieceTypeTo, WHITE));
                }

                if (Info->Type & MOVE_PAWN_PROMOTE) {
                    AccumulatorAdd(Board, Info->To, PIECE_CREATE(Info->PromotePieceType, BLACK));
                }
                else {
                    AccumulatorAdd(Board, Info->To, PIECE_CREATE(Info->PieceTypeFrom, BLACK));
                }

                AccumulatorSub(Board, Info->From, PIECE_CREATE(Info->PieceTypeFrom, BLACK));
            }
        } // if

        if (Info->Accumulator.AccumulatorActual) {
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
    } // for

    return FALSE;
}

I32 OutputLayer(const BoardItem* Board)
{
    I32 Result = OutputBias;

#ifdef USE_NNUE_AVX2
    __m256i ConstZero = _mm256_setzero_si256();

    __m256i Sum0 = ConstZero;
    __m256i Sum1 = ConstZero;

    __m256i* AccumulatorTile0 = (__m256i*)Board->Accumulator.Accumulator[Board->CurrentColor];
    __m256i* AccumulatorTile1 = (__m256i*)Board->Accumulator.Accumulator[CHANGE_COLOR(Board->CurrentColor)];

    __m256i* Weights0 = (__m256i*)&OutputWeights;
    __m256i* Weights1 = (__m256i*)&OutputWeights[HIDDEN_DIMENSION];

    for (int Reg = 0; Reg < NUM_REGS; Reg++) {
        __m256i Acc0 = _mm256_max_epi16(ConstZero, AccumulatorTile0[Reg]); // ReLU
        __m256i Acc1 = _mm256_max_epi16(ConstZero, AccumulatorTile1[Reg]); // ReLU

        Sum0 = _mm256_add_epi32(Sum0, _mm256_madd_epi16(Acc0, Weights0[Reg]));
        Sum1 = _mm256_add_epi32(Sum1, _mm256_madd_epi16(Acc1, Weights1[Reg]));
    }

    __m256i R8 = _mm256_add_epi32(Sum0, Sum1);
    __m128i R4 = _mm_add_epi32(_mm256_castsi256_si128(R8), _mm256_extractf128_si256(R8, 1));
    __m128i R2 = _mm_add_epi32(R4, _mm_srli_si128(R4, 8));
    __m128i R1 = _mm_add_epi32(R2, _mm_srli_si128(R2, 4));

    Result += _mm_cvtsi128_si32(R1);
#else
    for (int Index = 0; Index < HIDDEN_DIMENSION; Index++) {
        I16 Acc0 = MAX(0, Board->Accumulator.Accumulator[Board->CurrentColor][Index]); // ReLU
        I16 Acc1 = MAX(0, Board->Accumulator.Accumulator[CHANGE_COLOR(Board->CurrentColor)][Index]); // ReLU

        Result += Acc0 * OutputWeights[Index];
        Result += Acc1 * OutputWeights[HIDDEN_DIMENSION + Index];
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