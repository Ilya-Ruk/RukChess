// Def.h

#pragma once

#ifndef DEF_H
#define DEF_H

// Features (enable/disable)

// Debug

//#define DEBUG_BIT_BOARD_INIT
//#define DEBUG_MOVE
//#define DEBUG_HASH
//#define DEBUG_IID
//#define DEBUG_SINGULAR_EXTENSION
//#define DEBUG_SEE
//#define DEBUG_NNUE

// Common

#define ASPIRATION_WINDOW

#define COUNTER_MOVE_HISTORY

#define HASH_PREFETCH

//#define BIND_THREAD_V1
//#define BIND_THREAD_V2                        // Max. 64 CPUs

//#define USE_STATISTIC

//#define PRINT_CURRENT_MOVE                    // For UCI in root node

// Search

#define MATE_DISTANCE_PRUNING
#define REVERSE_FUTILITY_PRUNING
#define RAZORING
#define NULL_MOVE_PRUNING
#define PROBCUT
#define IID
#define KILLER_MOVE                             // Two killer moves
#define COUNTER_MOVE
#define BAD_CAPTURE_LAST
#define CHECK_EXTENSION
#define SINGULAR_EXTENSION
#define FUTILITY_PRUNING
#define LATE_MOVE_PRUNING
#define SEE_QUIET_MOVE_PRUNING
#define SEE_CAPTURE_MOVE_PRUNING
#define LATE_MOVE_REDUCTION

// Quiescence search

#define QUIESCENCE_USE_CHECK

#define QUIESCENCE_MATE_DISTANCE_PRUNING
#define QUIESCENCE_SEE_MOVE_PRUNING

// NNUE

#define USE_NNUE_AVX2
#define USE_NNUE_UPDATE

//#define PRINT_MIN_MAX_VALUES
//#define PRINT_WEIGHT_INDEX
//#define PRINT_ACCUMULATOR

// ------------------------------------------------------------------

// Parameter values

// Program name, program version, evaluation function name and copyright information

#define PROGRAM_NAME                            "RukChess"
#define PROGRAM_VERSION                         "4.0.1"

/*
    https://github.com/Ilya-Ruk/RukChessTrainer
    https://github.com/Ilya-Ruk/RukChessNets
*/
#define EVALUATION_FUNCTION_NAME                "NNUE2"

#define YEARS                                   "1999-2024"
#define AUTHOR                                  "Ilya Rukavishnikov"

// Max. limits and default values

#define MAX_PLY                                 128

#define MAX_GEN_MOVES                           256     // The maximum number of moves in the position
#define MAX_GAME_MOVES                          1024    // The maximum number of moves in the game

#define MAX_FEN_LENGTH                          256

#define ASPIRATION_WINDOW_START_DEPTH           4
#define ASPIRATION_WINDOW_INIT_DELTA            17

#define DEFAULT_HASH_TABLE_SIZE                 512     // Mbyte
#define MAX_HASH_TABLE_SIZE                     4096    // Mbyte

#define DEFAULT_THREADS                         1
#define MAX_THREADS                             64

#define DEFAULT_BOOK_FILE_NAME                  "book.txt"
#define DEFAULT_NNUE_FILE_NAME                  "net-3ba7af1fe396.nnue" // 05.12.2024

// Time management (Xiphos)

#define MAX_TIME                                86400   // Maximum time per move, seconds

#define DEFAULT_REDUCE_TIME                     25      // msec.
#define MAX_REDUCE_TIME                         1000    // msec.

#define MAX_MOVES_TO_GO                         25
#define MAX_TIME_MOVES_TO_GO                    3

#define MAX_TIME_STEPS                          10

#define MIN_TIME_RATIO                          0.5
#define MAX_TIME_RATIO                          1.2

#define MIN_SEARCH_DEPTH                        4

#endif // !DEF_H