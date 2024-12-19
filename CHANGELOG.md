# RukChess change log

## RukChess 4.0.1 (19.12.2024)

1. Time control bugfix (https://github.com/Ilya-Ruk/RukChess/issues/5)
2. Added UCI option ReduceTime
3. Corrected the code

## RukChess 4.0.0 (10.12.2024)

1. OMP bugfix
2. Using C17
3. Deleted PV-splitting
4. Deleted Root splitting
5. Deleted ABDADA
6. Deleted common tables
7. Lazy SMP is default
8. Deleted simplified evaluation function and sort moves by piece`s square score
9. Deleted sort moves by SEE
10. Alpha-beta pruning with MVV/LVA and heuristic is default
11. Deleted NNUE evaluation function (old Stockfish net)
12. Deleted Adam SGD tuning
13. Quiescence search is default
14. NegaScout is default
15. Using hash move and hash score is default
16. NNUE evaluation function is default (deleted the Toga evaluation function with tuning code)
17. Deleted PVS sorting
18. Deleted LAST_LAYER_AS_FLOAT
19. Deleted MCTS
20. Mate distance pruning after draw conditions and if not root node
21. Added built-in performance test NNUE
22. Save best moves only in master thread (function Search)
23. Deleted use SaveBestMoves function from function QuiescenceSearch
24. Changed MAX_THREADS from 128 to 64
25. Corrected ComputerMove function
26. Added allocate memory error check
27. Changed time control from 65535 to 32767 nodes
28. Corrected killer move
29. Corrected aspiration window search
30. Changed MIN_BOOK_GAMES from 50 to 30
31. Null move bugfix
32. ProbCut bugfix
33. Corrected singular extension (added condition !SkipMove)
34. Corrected the code
35. Reduced hidden layer from 512 to 256
36. New neural net "[Net 159, Epoch 077, Hash 0x3ba7af1fe396](https://github.com/Ilya-Ruk/RukChessNets/blob/master/Nets%20159/rukchess_077.nnue)"
37. New book file (587604 positions)

## RukChess 3.0.19 (08.05.2024)

1. Added COUNTER_MOVE_HISTORY
2. Corrected COUNTER_MOVE

## RukChess 3.0.18 (16.12.2023)

1. Added optimization for speed
2. Corrected the code
3. Deleted QUIESCENCE_CHECK_EXTENSION_EXTENDED (not used)
4. Converted some C++ code to C
5. Project updated to Microsoft VS 2022
6. Deleted IIR (not used)
7. Added the ability to specify the name of the NNUE file in a parameter to the program
8. New neural net "[Net 122, Epoch 081, Hash 0x7cf57d4dc994](https://github.com/Ilya-Ruk/RukChessNets/blob/master/Nets%20122/rukchess_081.nnue)"
9. New book file (378640 positions)

## RukChess 3.0.17 (17.11.2023)

1. Corrected and enabled ProbCut
2. Added ProbCut for ABDADA
3. Added UCI option BookFile
4. Added UCI option NnueFile

## RukChess 3.0.16 (23.10.2023)

1. Changed QUANTIZATION_PRECISION_IN from 32 to 64
2. Changed type of OutputBias from I16 to I32
3. Multipling OutputBias by QUANTIZATION_PRECISION_IN in ReadNetwork
4. Added search algorithm name to program name
5. Refactored Monte Carlo tree search (just for experimentation)
6. Cleaned up the code
7. New neural net "[Net 110, Epoch 022, Hash 0x6e5001eb7720](https://github.com/Ilya-Ruk/RukChessNets/blob/master/Nets%20110/rukchess_022.nnue)"

## RukChess 3.0.15 (05.02.2023)

1. Refactored the code
2. Corrected the code
3. Cleaned up the code
4. New neural net "[Net 070, Epoch 120, Hash 0x755b16a94877](https://github.com/Ilya-Ruk/RukChessNets/blob/master/Nets%20070/rukchess_120.nnue)"

## RukChess 3.0.14 (16.01.2023)

1. Added Monte Carlo tree search (just for experimentation)
2. Cleaned up the code

## RukChess 3.0.13 (01.01.2023)

1. Reverted correct SEE
2. Added CHANGELOG.md

## RukChess 3.0.12 (28.12.2022)

1. Removed unused code
2. Corrected the code
3. Formatted the code

## RukChess 3.0.11 (24.12.2022)

1. Disabled TuningAdamSGD
2. Corrected the code
3. Formatted the code

## RukChess 3.0.10 (21.12.2022)

1. Fixed bug in TuningAdamSGD
2. Corrected the code

## RukChess 3.0.9 (20.12.2022)

Fixed bug in TuningAdamSGD

## RukChess 3.0.8 (20.12.2022)

Refactored tuning code

## RukChess 3.0.7 (18.12.2022)

Removed king side (KS) and king quadrant (KQ) architectures from NNUE2

## RukChess 3.0.6 (17.12.2022)

1. Added evaluation function name to program name
2. Cleaned up the code

## RukChess 3.0.5 (16.11.2022)

Cleaned up the code

## RukChess 3.0.4 (01.11.2022)

Cleaned up the code

## RukChess 3.0.3 (18.10.2022)

Optimized NNUE2

## RukChess 3.0.2 (14.10.2022)

Optimized NNUE2

## RukChess 3.0.1 (03.10.2022)

1. Optimized SEE
2. Optimized search
3. Fixed bug in LazySMP
4. Added README.md
5. Added LICENSE
6. Formatted the code
7. Cleaned up the code

## RukChess 3.0 (05.09.2022)

1. Initial commit
2. Neural net "[Net 001, Epoch 040, Hash 0x71eb63511cb1](https://github.com/Ilya-Ruk/RukChessNets/blob/master/Nets%20001/rukchess_040.nnue)"
