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
//#define DEBUG_PVS
//#define DEBUG_SINGULAR_EXTENSION
//#define DEBUG_SEE

//#define DEBUG_STATISTIC

// Monte Carlo tree search

/*
    https://homes.di.unimi.it/~cesabian/Pubblicazioni/ml-02.pdf
    http://ggp.stanford.edu/readings/uct.pdf
    http://old.sztaki.hu/~szcsaba/papers/cg06-ext.pdf
    https://netman.aiops.org/~peidan/ANM2017/7.AnomalyLocalization/LectureCoverage/mcts-survey-master-origin.pdf
    https://int8.io/monte-carlo-tree-search-beginners-guide/
    https://github.com/int8/gomcts
    http://www.tckerrigan.com/Chess/TSCP/Community/tscp_mcts_export.zip
*/
//#define MCTS

// Common

#define ASPIRATION_WINDOW

#define COUNTER_MOVE_HISTORY

#define HASH_PREFETCH

// Search

#define MATE_DISTANCE_PRUNING
#define REVERSE_FUTILITY_PRUNING
#define RAZORING
#define NULL_MOVE_PRUNING
#define PROBCUT
#define IID
//#define PVS
#define KILLER_MOVE
#define COUNTER_MOVE                            // Required KILLER_MOVE
#define BAD_CAPTURE_LAST
#define CHECK_EXTENSION
#define SINGULAR_EXTENSION
#define FUTILITY_PRUNING
#define LATE_MOVE_PRUNING
#define SEE_QUIET_MOVE_PRUNING
#define SEE_CAPTURE_MOVE_PRUNING                // Required BAD_CAPTURE_LAST
#define LATE_MOVE_REDUCTION

// Quiescence search

#define QUIESCENCE_MATE_DISTANCE_PRUNING
#define QUIESCENCE_CHECK_EXTENSION
//#define QUIESCENCE_PVS
#define QUIESCENCE_SEE_MOVE_PRUNING

//#define BIND_THREAD
//#define BIND_THREAD_V2                        // Max. 64 CPUs

// ------------------------------------------------------------------

// Parameter values

// Program name, program version, algorithm name, evaluation function name and copyright information

#define PROGRAM_NAME                            "RukChess"
#define PROGRAM_VERSION                         "4.0.0dev"

#ifdef MCTS
#define ALGORITHM_NAME                          "MCTS"
#else // PVS
#define ALGORITHM_NAME                          "PVS"
#endif // MCTS || PVS

#define YEARS                                   "1999-2024"
#define AUTHOR                                  "Ilya Rukavishnikov"

// Max. limits and default values

#define MAX_PLY                                 128

#define MAX_GEN_MOVES                           256     // The maximum number of moves in the position
#define MAX_GAME_MOVES                          1024    // The maximum number of moves in the game

#define MAX_FEN_LENGTH                          256

#define ASPIRATION_WINDOW_INIT_DELTA            17

#define DEFAULT_HASH_TABLE_SIZE                 512     // Mbyte
#define MAX_HASH_TABLE_SIZE                     4096    // Mbyte

#define DEFAULT_THREADS                         1
#define MAX_THREADS                             128

#define DEFAULT_BOOK_FILE_NAME                  "book.txt"

// Time management (Xiphos)

#define MAX_TIME                                86400   // Maximum time per move, seconds

#define REDUCE_TIME                             150     // msec.

#define REDUCE_TIME_PERCENT                     5       // %
#define MAX_REDUCE_TIME                         1000    // msec.

#define MAX_MOVES_TO_GO                         40      // 25
#define MAX_TIME_MOVES_TO_GO                    3

#define MAX_TIME_STEPS                          10

#define MIN_TIME_RATIO                          0.5
#define MAX_TIME_RATIO                          1.2

#define MIN_SEARCH_DEPTH                        4

#endif // !DEF_H