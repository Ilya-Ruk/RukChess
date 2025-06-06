// Tests.cpp

#include "stdafx.h"

#include "Tests.h"

#include "Board.h"
#include "Def.h"
#include "Game.h"
#include "Hash.h"
#include "NNUE2.h"
#include "Types.h"
#include "Utils.h"

/*
    Tests 1-6 (https://www.chessprogramming.org/Perft_Results)
    Test 7 (http://www.rocechess.ch/perft.html)
*/
const GeneratorTestItem GeneratorTests[7] = {
    {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        7,
        {
            20ULL,
            400ULL,
            8902ULL,
            197281ULL,
            4865609ULL,
            119060324ULL,
            3195901860ULL
        }
    },

    {
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        6,
        {
            48ULL,
            2039ULL,
            97862ULL,
            4085603ULL,
            193690690ULL,
            8031647685ULL
        }
    },

    {
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        8,
        {
            14ULL,
            191ULL,
            2812ULL,
            43238ULL,
            674624ULL,
            11030083ULL,
            178633661ULL,
            3009794393ULL
        }
    },

    {
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        6,
        {
            6ULL,
            264ULL,
            9467ULL,
            422333ULL,
            15833292ULL,
            706045033ULL
        }
    },

    {
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        5,
        {
            44ULL,
            1486ULL,
            62379ULL,
            2103487ULL,
            89941194ULL
        }
    },

    {
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        6,
        {
            46ULL,
            2079ULL,
            89890ULL,
            3894594ULL,
            164075551ULL,
            6923051137ULL
        }
    },

    {
        "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1",
        6,
        {
            24ULL,
            496ULL,
            9483ULL,
            182838ULL,
            3605103ULL,
            71179139ULL
        }
    }
};

const int GeneratorTestCount = sizeof(GeneratorTests) / sizeof(GeneratorTestItem);

/*
    https://www.chessprogramming.org/Bratko-Kopec_Test
*/
const char* BratkoKopecTests[24 * 2] = {
    "1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b - - 0 1", "Qd1+",
    "3r1k2/4npp1/1ppr3p/p6P/P2PPPP1/1NR5/5K2/2R5 w - - 0 1", "d5",
    "2q1rr1k/3bbnnp/p2p1pp1/2pPp3/PpP1P1P1/1P2BNNP/2BQ1PRK/7R b - - 0 1", "f5",
    "rnbqkb1r/p3pppp/1p6/2ppP3/3N4/2P5/PPP1QPPP/R1B1KB1R w KQkq - 0 1", "e6",
    "r1b2rk1/2q1b1pp/p2ppn2/1p6/3QP3/1BN1B3/PPP3PP/R4RK1 w - - 0 1", "Nd5 a4",
    "2r3k1/pppR1pp1/4p3/4P1P1/5P2/1P4K1/P1P5/8 w - - 0 1", "g6",
    "1nk1r1r1/pp2n1pp/4p3/q2pPp1N/b1pP1P2/B1P2R2/2P1B1PP/R2Q2K1 w - - 0 1", "Nf6",
    "4b3/p3kp2/6p1/3pP2p/2pP1P2/4K1P1/P3N2P/8 w - - 0 1", "f5",
    "2kr1bnr/pbpq4/2n1pp2/3p3p/3P1P1B/2N2N1Q/PPP3PP/2KR1B1R w - - 0 1", "f5",
    "3rr1k1/pp3pp1/1qn2np1/8/3p4/PP1R1P2/2P1NQPP/R1B3K1 b - - 0 1", "Ne5",
    "2r1nrk1/p2q1ppp/bp1p4/n1pPp3/P1P1P3/2PBB1N1/4QPPP/R4RK1 w - - 0 1", "f4",
    "r3r1k1/ppqb1ppp/8/4p1NQ/8/2P5/PP3PPP/R3R1K1 b - - 0 1", "Bf5",
    "r2q1rk1/4bppp/p2p4/2pP4/3pP3/3Q4/PP1B1PPP/R3R1K1 w - - 0 1", "b4",
    "rnb2r1k/pp2p2p/2pp2p1/q2P1p2/8/1Pb2NP1/PB2PPBP/R2Q1RK1 w - - 0 1", "Qd2 Qe1",
    "2r3k1/1p2q1pp/2b1pr2/p1pp4/6Q1/1P1PP1R1/P1PN2PP/5RK1 w - - 0 1", "Qxg7+",
    "r1bqkb1r/4npp1/p1p4p/1p1pP1B1/8/1B6/PPPN1PPP/R2Q1RK1 w kq - 0 1", "Ne4",
    "r2q1rk1/1ppnbppp/p2p1nb1/3Pp3/2P1P1P1/2N2N1P/PPB1QP2/R1B2RK1 b - - 0 1", "h5",
    "r1bq1rk1/pp2ppbp/2np2p1/2n5/P3PP2/N1P2N2/1PB3PP/R1B1QRK1 b - - 0 1", "Nb3",
    "3rr3/2pq2pk/p2p1pnp/8/2QBPP2/1P6/P5PP/4RRK1 b - - 0 1", "Rxe4",
    "r4k2/pb2bp1r/1p1qp2p/3pNp2/3P1P2/2N3P1/PPP1Q2P/2KRR3 w - - 0 1", "g4",
    "3rn2k/ppb2rpp/2ppqp2/5N2/2P1P3/1P5Q/PB3PPP/3RR1K1 w - - 0 1", "Nh6",
    "2r2rk1/1bqnbpp1/1p1ppn1p/pP6/N1P1P3/P2B1N1P/1B2QPP1/R2R2K1 b - - 0 1", "Bxe4",
    "r1bqk2r/pp2bppp/2p5/3pP3/P2Q1P2/2N1B3/1PP3PP/R4RK1 b kq - 0 1", "f6",
    "r2qnrnk/p2b2b1/1p1p2pp/2pPpp2/1PP1P3/PRNBB3/3QNPPP/5RK1 w - - 0 1", "f4"
};

const int BratkoKopecTestCount = sizeof(BratkoKopecTests) / sizeof(char*) / 2;

/*
    https://www.chessprogramming.org/Win_at_Chess
*/
const char* WinAtChessTests[300 * 2] = {
    "2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - 0 1", "Qg6",
    "8/7p/5k2/5p2/p1p2P2/Pr1pPK2/1P1R3P/8 b - - 0 1", "Rxb2",
    "5rk1/1ppb3p/p1pb4/6q1/3P1p1r/2P1R2P/PP1BQ1P1/5RKN w - - 0 1", "Rg3",
    "r1bq2rk/pp3pbp/2p1p1pQ/7P/3P4/2PB1N2/PP3PPR/2KR4 w - - 0 1", "Qxh7+",
    "5k2/6pp/p1qN4/1p1p4/3P4/2PKP2Q/PP3r2/3R4 b - - 0 1", "Qc4+",
    "7k/p7/1R5K/6r1/6p1/6P1/8/8 w - - 0 1", "Rb7",
    "rnbqkb1r/pppp1ppp/8/4P3/6n1/7P/PPPNPPP1/R1BQKBNR b KQkq - 0 1", "Ne3",
    "r4q1k/p2bR1rp/2p2Q1N/5p2/5p2/2P5/PP3PPP/R5K1 w - - 0 1", "Rf7",
    "3q1rk1/p4pp1/2pb3p/3p4/6Pr/1PNQ4/P1PB1PP1/4RRK1 b - - 0 1", "Bh2+",
    "2br2k1/2q3rn/p2NppQ1/2p1P3/Pp5R/4P3/1P3PPP/3R2K1 w - - 0 1", "Rxh7",
    "r1b1kb1r/3q1ppp/pBp1pn2/8/Np3P2/5B2/PPP3PP/R2Q1RK1 w kq - 0 1", "Bxc6",
    "4k1r1/2p3r1/1pR1p3/3pP2p/3P2qP/P4N2/1PQ4P/5R1K b - - 0 1", "Qxf3+",
    "5rk1/pp4p1/2n1p2p/2Npq3/2p5/6P1/P3P1BP/R4Q1K w - - 0 1", "Qxf8+",
    "r2rb1k1/pp1q1p1p/2n1p1p1/2bp4/5P2/PP1BPR1Q/1BPN2PP/R5K1 w - - 0 1", "Qxh7+",
    "1R6/1brk2p1/4p2p/p1P1Pp2/P7/6P1/1P4P1/2R3K1 w - - 0 1", "Rxb7",
    "r4rk1/ppp2ppp/2n5/2bqp3/8/P2PB3/1PP1NPPP/R2Q1RK1 w - - 0 1", "Nc3",
    "1k5r/pppbn1pp/4q1r1/1P3p2/2NPp3/1QP5/P4PPP/R1B1R1K1 w - - 0 1", "Ne5",
    "R7/P4k2/8/8/8/8/r7/6K1 w - - 0 1", "Rh8",
    "r1b2rk1/ppbn1ppp/4p3/1QP4q/3P4/N4N2/5PPP/R1B2RK1 w - - 0 1", "c6",
    "r2qkb1r/1ppb1ppp/p7/4p3/P1Q1P3/2P5/5PPP/R1B2KNR b kq - 0 1", "Bb5",
    "5rk1/1b3p1p/pp3p2/3n1N2/1P6/P1qB1PP1/3Q3P/4R1K1 w - - 0 1", "Qh6",
    "r1bqk2r/ppp1nppp/4p3/n5N1/2BPp3/P1P5/2P2PPP/R1BQK2R w KQkq - 0 1", "Ba2 Nxf7",
    "r3nrk1/2p2p1p/p1p1b1p1/2NpPq2/3R4/P1N1Q3/1PP2PPP/4R1K1 w - - 0 1", "g4",
    "6k1/1b1nqpbp/pp4p1/5P2/1PN5/4Q3/P5PP/1B2B1K1 b - - 0 1", "Bd4",
    "3R1rk1/8/5Qpp/2p5/2P1p1q1/P3P3/1P2PK2/8 b - - 0 1", "Qh4+",
    "3r2k1/1p1b1pp1/pq5p/8/3NR3/2PQ3P/PP3PP1/6K1 b - - 0 1", "Bf5",
    "7k/pp4np/2p3p1/3pN1q1/3P4/Q7/1r3rPP/2R2RK1 w - - 0 1", "Qf8+",
    "1r1r2k1/4pp1p/2p1b1p1/p3R3/RqBP4/4P3/1PQ2PPP/6K1 b - - 0 1", "Qe1+",
    "r2q2k1/pp1rbppp/4pn2/2P5/1P3B2/6P1/P3QPBP/1R3RK1 w - - 0 1", "c6",
    "1r3r2/4q1kp/b1pp2p1/5p2/pPn1N3/6P1/P3PPBP/2QRR1K1 w - - 0 1", "Nxd6",
    "rb3qk1/pQ3ppp/4p3/3P4/8/1P3N2/1P3PPP/3R2K1 w - - 0 1", "Qxa8 d6 dxe6 g3",
    "6k1/p4p1p/1p3np1/2q5/4p3/4P1N1/PP3PPP/3Q2K1 w - - 0 1", "Qd8+",
    "8/p1q2pkp/2Pr2p1/8/P3Q3/6P1/5P1P/2R3K1 w - - 0 1", "Qe5+ Qf4",
    "7k/1b1r2p1/p6p/1p2qN2/3bP3/3Q4/P5PP/1B1R3K b - - 0 1", "Bg1",
    "r3r2k/2R3pp/pp1q1p2/8/3P3R/7P/PP3PP1/3Q2K1 w - - 0 1", "Rxh7+",
    "3r4/2p1rk2/1pQq1pp1/7p/1P1P4/P4P2/6PP/R1R3K1 b - - 0 1", "Re1+",
    "2r5/2rk2pp/1pn1pb2/pN1p4/P2P4/1N2B3/nPR1KPPP/3R4 b - - 0 1", "Nxd4+",
    "4k3/p4prp/1p6/2b5/8/2Q3P1/P2R1PKP/4q3 w - - 0 1", "Qd3 Rd8+",
    "r1br2k1/pp2bppp/2nppn2/8/2P1PB2/2N2P2/PqN1B1PP/R2Q1R1K w - - 0 1", "Na4",
    "3r1r1k/1p4pp/p4p2/8/1PQR4/6Pq/P3PP2/2R3K1 b - - 0 1", "Rc8",
    "1k6/5RP1/1P6/1K6/6r1/8/8/8 w - - 0 1", "Ka5 Kc5 b7",
    "r1b1r1k1/pp1n1pbp/1qp3p1/3p4/1B1P4/Q3PN2/PP2BPPP/R4RK1 w - - 0 1", "Ba5",
    "r2q3k/p2P3p/1p3p2/3QP1r1/8/B7/P5PP/2R3K1 w - - 0 1", "Be7 Qxa8",
    "3rb1k1/pq3pbp/4n1p1/3p4/2N5/2P2QB1/PP3PPP/1B1R2K1 b - - 0 1", "dxc4",
    "7k/2p1b1pp/8/1p2P3/1P3r2/2P3Q1/1P5P/R4qBK b - - 0 1", "Qxa1",
    "r1bqr1k1/pp1nb1p1/4p2p/3p1p2/3P4/P1N1PNP1/1PQ2PP1/3RKB1R w K - 0 1", "Nb5",
    "r1b2rk1/pp2bppp/2n1pn2/q5B1/2BP4/2N2N2/PP2QPPP/2R2RK1 b - - 0 1", "Nxd4",
    "1rbq1rk1/p1p1bppp/2p2n2/8/Q1BP4/2N5/PP3PPP/R1B2RK1 b - - 0 1", "Rb4",
    "2b3k1/4rrpp/p2p4/2pP2RQ/1pP1Pp1N/1P3P1P/1q6/6RK w - - 0 1", "Qxh7+",
    "k4r2/1R4pb/1pQp1n1p/3P4/5p1P/3P2P1/r1q1R2K/8 w - - 0 1", "Rxb6+",
    "r1bq1r2/pp4k1/4p2p/3pPp1Q/3N1R1P/2PB4/6P1/6K1 w - - 0 1", "Rg4+",
    "r1k5/1p3q2/1Qpb4/3N1p2/5Pp1/3P2Pp/PPPK3P/4R3 w - - 0 1", "Re7 c4",
    "6k1/6p1/p7/3Pn3/5p2/4rBqP/P4RP1/5QK1 b - - 0 1", "Re1",
    "r3kr2/1pp4p/1p1p4/7q/4P1n1/2PP2Q1/PP4P1/R1BB2K1 b q - 0 1", "Qh1+",
    "r3r1k1/pp1q1pp1/4b1p1/3p2B1/3Q1R2/8/PPP3PP/4R1K1 w - - 0 1", "Qxg7+",
    "r1bqk2r/pppp1ppp/5n2/2b1n3/4P3/1BP3Q1/PP3PPP/RNB1K1NR b KQkq - 0 1", "Bxf2+",
    "r3q1kr/ppp5/3p2pQ/8/3PP1b1/5R2/PPP3P1/5RK1 w - - 0 1", "Rf8+",
    "8/8/2R5/1p2qp1k/1P2r3/2PQ2P1/5K2/8 w - - 0 1", "Qd1+",
    "r1b2rk1/2p1qnbp/p1pp2p1/5p2/2PQP3/1PN2N1P/PB3PP1/3R1RK1 w - - 0 1", "Nd5",
    "rn1qr1k1/1p2np2/2p3p1/8/1pPb4/7Q/PB1P1PP1/2KR1B1R w - - 0 1", "Qh8+",
    "3qrbk1/ppp1r2n/3pP2p/3P4/2P4P/1P3Q2/PB6/R4R1K w - - 0 1", "Qf7+",
    "6r1/3Pn1qk/p1p1P1rp/2Q2p2/2P5/1P4P1/P3R2P/5RK1 b - - 0 1", "Rxg3+",
    "r1brnbk1/ppq2pp1/4p2p/4N3/3P4/P1PB1Q2/3B1PPP/R3R1K1 w - - 0 1", "Nxf7",
    "8/6pp/3q1p2/3n1k2/1P6/3NQ2P/5PP1/6K1 w - - 0 1", "g4+",
    "1r1r1qk1/p2n1p1p/bp1Pn1pQ/2pNp3/2P2P1N/1P5B/P6P/3R1RK1 w - - 0 1", "Ne7+",
    "1k1r2r1/ppq5/1bp4p/3pQ3/8/2P2N2/PP4P1/R4R1K b - - 0 1", "Qxe5",
    "3r2k1/p2q4/1p4p1/3rRp1p/5P1P/6PK/P3R3/3Q4 w - - 0 1", "Rxd5",
    "6k1/5ppp/1q6/2b5/8/2R1pPP1/1P2Q2P/7K w - - 0 1", "Qxe3",
    "2k5/pppr4/4R3/4Q3/2pp2q1/8/PPP2PPP/6K1 w - - 0 1", "f3 h3",
    "2kr3r/pppq1ppp/3p1n2/bQ2p3/1n1PP3/1PN1BN1P/1PP2PP1/2KR3R b - - 0 1", "Na2+",
    "2kr3r/pp1q1ppp/5n2/1Nb5/2Pp1B2/7Q/P4PPP/1R3RK1 w - - 0 1", "Nxa7+",
    "r3r1k1/pp1n1ppp/2p5/4Pb2/2B2P2/B1P5/P5PP/R2R2K1 w - - 0 1", "e6",
    "r1q3rk/1ppbb1p1/4Np1p/p3pP2/P3P3/2N4R/1PP1Q1PP/3R2K1 w - - 0 1", "Qd2",
    "5r1k/pp4pp/2p5/2b1P3/4Pq2/1PB1p3/P3Q1PP/3N2K1 b - - 0 1", "Qf1+",
    "r3r1k1/pppq1ppp/8/8/1Q4n1/7P/PPP2PP1/RNB1R1K1 b - - 0 1", "Qd6",
    "r1b1qrk1/2p2ppp/pb1pnn2/1p2pNB1/3PP3/1BP5/PP2QPPP/RN1R2K1 w - - 0 1", "Bxf6",
    "3r2k1/ppp2ppp/6q1/b4n2/3nQB2/2p5/P4PPP/RN3RK1 b - - 0 1", "Ng3",
    "r2q3r/ppp2k2/4nbp1/5Q1p/2P1NB2/8/PP3P1P/3RR1K1 w - - 0 1", "Ng5+",
    "r3k2r/pbp2pp1/3b1n2/1p6/3P3p/1B2N1Pq/PP1PQP1P/R1B2RK1 b kq - 0 1", "Qxh2+",
    "r4rk1/p1B1bpp1/1p2pn1p/8/2PP4/3B1P2/qP2QP1P/3R1RK1 w - - 0 1", "Ra1",
    "r4rk1/1bR1bppp/4pn2/1p2N3/1P6/P3P3/4BPPP/3R2K1 b - - 0 1", "Bd6",
    "3rr1k1/pp3pp1/4b3/8/2P1B2R/6QP/P3q1P1/5R1K w - - 0 1", "Bh7+",
    "3rr1k1/ppqbRppp/2p5/8/3Q1n2/2P3N1/PPB2PPP/3R2K1 w - - 0 1", "Qxd7",
    "r2q1r1k/2p1b1pp/p1n5/1p1Q1bN1/4n3/1BP1B3/PP3PPP/R4RK1 w - - 0 1", "Qg8+",
    "kr2R3/p4r2/2pq4/2N2p1p/3P2p1/Q5P1/5P1P/5BK1 w - - 0 1", "Na6",
    "8/p7/1ppk1n2/5ppp/P1PP4/2P1K1P1/5N1P/8 b - - 0 1", "Ng4+",
    "8/p3k1p1/4r3/2ppNpp1/PP1P4/2P3KP/5P2/8 b - - 0 1", "Rxe5",
    "r6k/p1Q4p/2p1b1rq/4p3/B3P3/4P3/PPP3P1/4RRK1 b - - 0 1", "Rxg2+",
    "1r3b1k/p4rpp/4pp2/3q4/2ppbPPQ/6RK/PP5P/2B1NR2 b - - 0 1", "g5",
    "3qrrk1/1pp2pp1/1p2bn1p/5N2/2P5/P1P3B1/1P4PP/2Q1RRK1 w - - 0 1", "Nxg7",
    "2qr2k1/4b1p1/2p2p1p/1pP1p3/p2nP3/PbQNB1PP/1P3PK1/4RB2 b - - 0 1", "Be6",
    "r4rk1/1p2ppbp/p2pbnp1/q7/3BPPP1/2N2B2/PPP4P/R2Q1RK1 b - - 0 1", "Bxg4",
    "r1b1k1nr/pp3pQp/4pq2/3pn3/8/P1P5/2P2PPP/R1B1KBNR w KQkq - 0 1", "Bh6",
    "8/k7/p7/3Qp2P/n1P5/3KP3/1q6/8 b - - 0 1", "e4+",
    "2r5/1r6/4pNpk/3pP1qp/8/2P1QP2/5PK1/R7 w - - 0 1", "Ng4+",
    "r1b4k/ppp2Bb1/6Pp/3pP3/1qnP1p1Q/8/PPP3P1/1K1R3R w - - 0 1", "Qd8+ b3",
    "6k1/5p2/p5np/4B3/3P4/1PP1q3/P3r1QP/6RK w - - 0 1", "Qa8+",
    "1r3rk1/5pb1/p2p2p1/Q1n1q2p/1NP1P3/3p1P1B/PP1R3P/1K2R3 b - - 0 1", "Nxe4",
    "r1bq1r1k/1pp1Np1p/p2p2pQ/4R3/n7/8/PPPP1PPP/R1B3K1 w - - 0 1", "Rh5",
    "8/k1b5/P4p2/1Pp2p1p/K1P2P1P/8/3B4/8 w - - 0 1", "Be3 b6+",
    "5rk1/p5pp/8/8/2Pbp3/1P4P1/7P/4RN1K b - - 0 1", "Bc3",
    "2Q2n2/2R4p/1p1qpp1k/8/3P3P/3B2P1/5PK1/r7 w - - 0 1", "Qxf8+",
    "6k1/2pb1r1p/3p1PpQ/p1nPp3/1q2P3/2N2P2/PrB5/2K3RR w - - 0 1", "Qxg6+",
    "b4r1k/pq2rp2/1p1bpn1p/3PN2n/2P2P2/P2B3K/1B2Q2N/3R2R1 w - - 0 1", "Qxh5",
    "r2r2k1/pb3ppp/1p1bp3/7q/3n2nP/PP1B2P1/1B1N1P2/RQ2NRK1 b - - 0 1", "Bxg3 Qxh4",
    "4rrk1/pppb4/7p/3P2pq/3Qn3/P5P1/1PP4P/R3RNNK b - - 0 1", "Nf2+",
    "5n2/pRrk2p1/P4p1p/4p3/3N4/5P2/6PP/6K1 w - - 0 1", "Nb5",
    "r5k1/1q4pp/2p5/p1Q5/2P5/5R2/4RKPP/r7 w - - 0 1", "Qe5",
    "rn2k1nr/pbp2ppp/3q4/1p2N3/2p5/QP6/PB1PPPPP/R3KB1R b KQkq - 0 1", "c3",
    "2kr4/bp3p2/p2p2b1/P7/2q5/1N4B1/1PPQ2P1/2KR4 b - - 0 1", "Be3",
    "6k1/p5p1/5p2/2P2Q2/3pN2p/3PbK1P/7P/6q1 b - - 0 1", "Qf1+",
    "r4kr1/ppp5/4bq1b/7B/2PR1Q1p/2N3P1/PP3P1P/2K1R3 w - - 0 1", "Rxe6",
    "rnbqkb1r/1p3ppp/5N2/1p2p1B1/2P5/8/PP2PPPP/R2QKB1R b KQkq - 0 1", "Qxf6",
    "r1b1rnk1/1p4pp/p1p2p2/3pN2n/3P1PPq/2NBPR1P/PPQ5/2R3K1 w - - 0 1", "Bxh7+",
    "4N2k/5rpp/1Q6/p3q3/8/P5P1/1P3P1P/5K2 w - - 0 1", "Nd6",
    "r2r2k1/2p2ppp/p7/1p2P1n1/P6q/5P2/1PB1QP1P/R5RK b - - 0 1", "Rd2",
    "3r1rk1/q4ppp/p1Rnp3/8/1p6/1N3P2/PP3QPP/3R2K1 b - - 0 1", "Ne4",
    "r5k1/pb2rpp1/1p6/2p4q/5R2/2PB2Q1/P1P3PP/5R1K w - - 0 1", "Rh4",
    "r2qr1k1/p1p2ppp/2p5/2b5/4nPQ1/3B4/PPP3PP/R1B2R1K b - - 0 1", "Qxd3",
    "r4rk1/1bn2qnp/3p1B1Q/p2P1pP1/1pp5/5N1P/PPB2P2/2KR3R w - - 0 1", "Rhg1 g6",
    "6k1/5p1p/2bP2pb/4p3/2P5/1p1pNPPP/1P1Q1BK1/1q6 b - - 0 1", "Bxf3+",
    "1k6/ppp4p/1n2pq2/1N2Rb2/2P2Q2/8/P4KPP/3r1B2 b - - 0 1", "Rxf1+",
    "6k1/1b2rp2/1p4p1/3P4/PQ4P1/2N2q2/5P2/3R2K1 b - - 0 1", "Bxd5 Rc7 Re6",
    "6k1/3r4/2R5/P5P1/1P4p1/8/4rB2/6K1 b - - 0 1", "g3",
    "r1bqr1k1/pp3ppp/1bp5/3n4/3B4/2N2P1P/PPP1B1P1/R2Q1RK1 b - - 0 1", "Bxd4+",
    "r5r1/pQ5p/1qp2R2/2k1p3/4P3/2PP4/P1P3PP/6K1 w - - 0 1", "Rxc6+",
    "2k4r/1pr1n3/p1p1q2p/5pp1/3P1P2/P1P1P3/1R2Q1PP/1RB3K1 w - - 0 1", "Rxb7",
    "6rk/1pp2Qrp/3p1B2/1pb1p2R/3n1q2/3P4/PPP3PP/R6K w - - 0 1", "Qg6",
    "3r1r1k/1b2b1p1/1p5p/2p1Pp2/q1B2P2/4P2P/1BR1Q2K/6R1 b - - 0 1", "Bf3",
    "6k1/1pp3q1/5r2/1PPp4/3P1pP1/3Qn2P/3B4/4R1K1 b - - 0 1", "Qh6 Qh8",
    "2rq1bk1/p4p1p/1p4p1/3b4/3B1Q2/8/P4PpP/3RR1K1 w - - 0 1", "Re8",
    "4r1k1/5bpp/2p5/3pr3/8/1B3pPq/PPR2P2/2R2QK1 b - - 0 1", "Re1",
    "r1b1k2r/1pp1q2p/p1n3p1/3QPp2/8/1BP3B1/P5PP/3R1RK1 w kq - 0 1", "Bh4",
    "3r2k1/p6p/2Q3p1/4q3/2P1p3/P3Pb2/1P3P1P/2K2BR1 b - - 0 1", "Rd1+",
    "3r1r1k/N2qn1pp/1p2np2/2p5/2Q1P2N/3P4/PP4PP/3R1RK1 b - - 0 1", "Nd4",
    "6kr/1q2r1p1/1p2N1Q1/5p2/1P1p4/6R1/7P/2R3K1 w - - 0 1", "Rc8+",
    "3b1rk1/1bq3pp/5pn1/1p2rN2/2p1p3/2P1B2Q/1PB2PPP/R2R2K1 w - - 0 1", "Rd7",
    "r1bq3r/ppppR1p1/5n1k/3P4/6pP/3Q4/PP1N1PP1/5K1R w - - 0 1", "h5",
    "rnb3kr/ppp2ppp/1b6/3q4/3pN3/Q4N2/PPP2KPP/R1B1R3 w - - 0 1", "Nf6+",
    "r2b1rk1/pq4p1/4ppQP/3pB1p1/3P4/2R5/PP3PP1/5RK1 w - - 0 1", "Bc7 Rc7",
    "4r1k1/p1qr1p2/2pb1Bp1/1p5p/3P1n1R/1B3P2/PP3PK1/2Q4R w - - 0 1", "Qxf4",
    "r2q3n/ppp2pk1/3p4/5Pr1/2NP1Qp1/2P2pP1/PP3K2/4R2R w - - 0 1", "Re8 f6+",
    "5b2/pp2r1pk/2pp1pRp/4rP1N/2P1P3/1P4QP/P3q1P1/5R1K w - - 0 1", "Rxh6+",
    "r2q1rk1/pp3ppp/2p2b2/8/B2pPPb1/7P/PPP1N1P1/R2Q1RK1 b - - 0 1", "d3",
    "r1bq4/1p4kp/3p1n2/p4pB1/2pQ4/8/1P4PP/4RRK1 w - - 0 1", "Re8",
    "8/8/2Kp4/3P1B2/2P2k2/5p2/8/8 w - - 0 1", "Bc8 Bd3 Bh3",
    "r2r2k1/ppqbppbp/2n2np1/2pp4/6P1/1P1PPNNP/PBP2PB1/R2QK2R b KQ - 0 1", "Nxg4",
    "2r1k3/6pr/p1nBP3/1p3p1p/2q5/2P5/P1R4P/K2Q2R1 w - - 0 1", "Rxg7",
    "6k1/6p1/2p4p/4Pp2/4b1qP/2Br4/1P2RQPK/8 b - - 0 1", "Bxg2",
    "r3r1k1/5p2/pQ1b2pB/1p6/4p3/6P1/Pq2BP1P/2R3K1 b - - 0 1", "Ba3 Be5 Bf8 e3",
    "8/3b2kp/4p1p1/pr1n4/N1N4P/1P4P1/1K3P2/3R4 w - - 0 1", "Nc3",
    "1br2rk1/1pqb1ppp/p3pn2/8/1P6/P1N1PN1P/1B3PP1/1QRR2K1 w - - 0 1", "Ne4",
    "2r3k1/q4ppp/p3p3/pnNp4/2rP4/2P2P2/4R1PP/2R1Q1K1 b - - 0 1", "Nxd4",
    "r1b2rk1/2p2ppp/p7/1p6/3P3q/1BP3bP/PP3QP1/RNB1R1K1 w - - 0 1", "Qxf7+",
    "5bk1/1rQ4p/5pp1/2pP4/3n1PP1/7P/1q3BB1/4R1K1 w - - 0 1", "d6",
    "r1b1qN1k/1pp3p1/p2p3n/4p1B1/8/1BP4Q/PP3KPP/8 w - - 0 1", "Qxh6+",
    "5rk1/p4ppp/2p1b3/3Nq3/4P1n1/1p1B2QP/1PPr2P1/1K2R2R w - - 0 1", "Ne7+",
    "5rk1/n1p1R1bp/p2p4/1qpP1QB1/7P/2P3P1/PP3P2/6K1 w - - 0 1", "Rxg7+",
    "r1b2r2/5P1p/ppn3pk/2p1p1Nq/1bP1PQ2/3P4/PB4BP/1R3RK1 w - - 0 1", "Ne6+",
    "qn1kr2r/1pRbb3/pP5p/P2pP1pP/3N1pQ1/3B4/3B1PP1/R5K1 w - - 0 1", "Qxd7+",
    "3r3k/3r1P1p/pp1Nn3/2pp4/7Q/6R1/Pq4PP/5RK1 w - - 0 1", "Qxd8+",
    "r3kbnr/p4ppp/2p1p3/8/Q1B3b1/2N1B3/PP3PqP/R3K2R w KQkq - 0 1", "Bd5",
    "5rk1/2p4p/2p4r/3P4/4p1b1/1Q2NqPp/PP3P1K/R4R2 b - - 0 1", "Qg2+",
    "8/6pp/4p3/1p1n4/1NbkN1P1/P4P1P/1PR3K1/r7 w - - 0 1", "Rxc4+",
    "1r5k/p1p3pp/8/8/4p3/P1P1R3/1P1Q1qr1/2KR4 w - - 0 1", "Re2",
    "r3r1k1/5pp1/p1p4p/2Pp4/8/q1NQP1BP/5PP1/4K2R b K - 0 1", "d4",
    "7Q/ppp2q2/3p2k1/P2Ppr1N/1PP5/7R/5rP1/6K1 b - - 0 1", "Rxg2+",
    "r3k2r/pb1q1p2/8/2p1pP2/4p1p1/B1P1Q1P1/P1P3K1/R4R2 b kq - 0 1", "Qd2+",
    "5rk1/1pp3bp/3p2p1/2PPp3/1P2P3/2Q1B3/4q1PP/R5K1 b - - 0 1", "Bh6",
    "5r1k/6Rp/1p2p3/p2pBp2/1qnP4/4P3/Q4PPP/6K1 w - - 0 1", "Qxc4",
    "2rq4/1b2b1kp/p3p1p1/1p1nNp2/7P/1B2B1Q1/PP3PP1/3R2K1 w - - 0 1", "Bh6+",
    "5r1k/p5pp/8/1P1pq3/P1p2nR1/Q7/5BPP/6K1 b - - 0 1", "Qe1+",
    "2r1b3/1pp1qrk1/p1n1P1p1/7R/2B1p3/4Q1P1/PP3PP1/3R2K1 w - - 0 1", "Qh6+",
    "2r2rk1/6p1/p3pq1p/1p1b1p2/3P1n2/PP3N2/3N1PPP/1Q2RR1K b - - 0 1", "Nxg2",
    "r5k1/pppb3p/2np1n2/8/3PqNpP/3Q2P1/PPP5/R4RK1 w - - 0 1", "Nh5",
    "r1bq3r/ppp2pk1/3p1pp1/8/2BbPQ2/2NP2P1/PPP4P/R4R1K b - - 0 1", "Rxh2+",
    "r1b3r1/4qk2/1nn1p1p1/3pPp1P/p4P2/1p3BQN/PKPBN3/3R3R b - - 0 1", "Qa3+",
    "3r2k1/p1rn1p1p/1p2pp2/6q1/3PQNP1/5P2/P1P4R/R5K1 w - - 0 1", "Nxe6",
    "r1b2r1k/pp4pp/3p4/3B4/8/1QN3Pn/PP3q1P/R3R2K b - - 0 1", "Qg1+",
    "r1q2rk1/p3bppb/3p1n1p/2nPp3/1p2P1P1/6NP/PP2QPB1/R1BNK2R b KQ - 0 1", "Nxd5",
    "r3k2r/2p2p2/p2p1n2/1p2p3/4P2p/1PPPPp1q/1P5P/R1N2QRK b kq - 0 1", "Ng4",
    "r1b2rk1/ppqn1p1p/2n1p1p1/2b3N1/2N5/PP1BP3/1B3PPP/R2QK2R w KQ - 0 1", "Qh5",
    "1r2k1r1/5p2/b3p3/1p2b1B1/3p3P/3B4/PP2KP2/2R3R1 w - - 0 1", "Bf6",
    "4kn2/r4p1r/p3bQ2/q1nNP1Np/1p5P/8/PPP3P1/2KR3R w - - 0 1", "Qe7+",
    "1r1rb1k1/2p3pp/p2q1p2/3PpP1Q/Pp1bP2N/1B5R/1P4PP/2B4K w - - 0 1", "Qxh7+",
    "r5r1/p1q2p1k/1p1R2pB/3pP3/6bQ/2p5/P1P1NPPP/6K1 w - - 0 1", "Bf8+",
    "6k1/5p2/p3p3/1p3qp1/2p1Qn2/2P1R3/PP1r1PPP/4R1K1 b - - 0 1", "Nh3+",
    "3RNbk1/pp3p2/4rQpp/8/1qr5/7P/P4P2/3R2K1 w - - 0 1", "Qg7+",
    "3r1k2/1ppPR1n1/p2p1rP1/3P3p/4Rp1N/5K2/P1P2P2/8 w - - 0 1", "Re8+",
    "8/p2b2kp/1q1p2p1/1P1Pp3/4P3/3B2P1/P2Q3P/2Nn3K b - - 0 1", "Bh3",
    "2r1Rn1k/1p1q2pp/p7/5p2/3P4/1B4P1/P1P1QP1P/6K1 w - - 0 1", "Qc4",
    "r3k3/ppp2Npp/4Bn2/2b5/1n1pp3/N4P2/PPP3qP/R2QKR2 b Qq - 0 1", "Nd3+",
    "5bk1/p4ppp/Qp6/4B3/1P6/Pq2P1P1/2rr1P1P/R4RK1 b - - 0 1", "Qxe3",
    "5rk1/ppq2ppp/2p5/4bN2/4P3/6Q1/PPP2PPP/3R2K1 w - - 0 1", "Nh6+",
    "3r1rk1/1p3p2/p3pnnp/2p3p1/2P2q2/1P5P/PB2QPPN/3RR1K1 w - - 0 1", "g3",
    "rr4k1/p1pq2pp/Q1n1pn2/2bpp3/4P3/2PP1NN1/PP3PPP/R1B1K2R b KQ - 0 1", "Nb4",
    "7k/1p4p1/7p/3P1n2/4Q3/2P2P2/PP3qRP/7K b - - 0 1", "Qf1+",
    "2br2k1/ppp2p1p/4p1p1/4P2q/2P1Bn2/2Q5/PP3P1P/4R1RK b - - 0 1", "Rd3",
    "r1br2k1/pp2nppp/2n5/1B1q4/Q7/4BN2/PP3PPP/2R2RK1 w - - 0 1", "Bxc6 Rcd1 Rfd1",
    "2rqrn1k/pb4pp/1p2pp2/n2P4/2P3N1/P2B2Q1/1B3PPP/2R1R1K1 w - - 0 1", "Bxf6",
    "2b2r1k/4q2p/3p2pQ/2pBp3/8/6P1/1PP2P1P/R5K1 w - - 0 1", "Ra7",
    "QR2rq1k/2p3p1/3p1pPp/8/4P3/8/P1r3PP/1R4K1 b - - 0 1", "Rxa2",
    "r4rk1/5ppp/p3q1n1/2p2NQ1/4n3/P3P3/1B3PPP/1R3RK1 w - - 0 1", "Qh6",
    "r1b1qrk1/1p3ppp/p1p5/3Nb3/5N2/P7/1P4PQ/K1R1R3 w - - 0 1", "Rxe5",
    "r3rnk1/1pq2bb1/p4p2/3p1Pp1/3B2P1/1NP4R/P1PQB3/2K4R w - - 0 1", "Qxg5",
    "1Qq5/2P1p1kp/3r1pp1/8/8/7P/p4PP1/2R3K1 b - - 0 1", "Rc6",
    "r1bq2kr/p1pp1ppp/1pn1p3/4P3/2Pb2Q1/BR6/P4PPP/3K1BNR w - - 0 1", "Qxg7+",
    "3r1bk1/ppq3pp/2p5/2P2Q1B/8/1P4P1/P6P/5RK1 w - - 0 1", "Bf7+",
    "4kb1r/2q2p2/r2p4/pppBn1B1/P6P/6Q1/1PP5/2KRR3 w k - 0 1", "Rxe5+",
    "3r1rk1/pp1q1ppp/3pn3/2pN4/5PP1/P5PQ/1PP1B3/1K1R4 w - - 0 1", "Rh1",
    "r1bqrk2/pp1n1n1p/3p1p2/P1pP1P1Q/2PpP1NP/6R1/2PB4/4RBK1 w - - 0 1", "Qxf7+",
    "rn1qr2Q/pbppk1p1/1p2pb2/4N3/3P4/2N5/PPP3PP/R4RK1 w - - 0 1", "Qxg7+",
    "3r1r1k/1b4pp/ppn1p3/4Pp1R/Pn5P/3P4/4QP2/1qB1NKR1 w - - 0 1", "Rxh7+",
    "r2r2k1/1p2qpp1/1np1p1p1/p3N3/2PPN3/bP5R/4QPPP/4R1K1 w - - 0 1", "Ng5",
    "3r2k1/pb1q1pp1/1p2pb1p/8/3N4/P2QB3/1P3PPP/1Br1R1K1 w - - 0 1", "Qh7+",
    "r2qr1k1/1b1nbppp/p3pn2/1p1pN3/3P1B2/2PB1N2/PP2QPPP/R4RK1 w - - 0 1", "Nxf7 a4",
    "r3kb1r/1pp3p1/p3bp1p/5q2/3QN3/1P6/PBP3P1/3RR1K1 w kq - 0 1", "Qd7+",
    "6k1/pp5p/2p3q1/6BP/2nPr1Q1/8/PP3R1K/8 w - - 0 1", "Bh6",
    "7k/p4q1p/1pb5/2p5/4B2Q/2P1B3/P6P/7K b - - 0 1", "Qf1+",
    "3rr1k1/ppp2ppp/8/5Q2/4n3/1B5R/PPP1qPP1/5RK1 b - - 0 1", "Qxf1+",
    "r3k3/P5bp/2N1bp2/4p3/2p5/6NP/1PP2PP1/3R2K1 w q - 0 1", "Rd8+",
    "2r1r2k/1q3ppp/p2Rp3/2p1P3/6QB/p3P3/bP3PPP/3R2K1 w - - 0 1", "Bf6",
    "r1bqk2r/pp3ppp/5n2/8/1b1npB2/2N5/PP1Q2PP/1K2RBNR w kq - 0 1", "Nxe4",
    "5rk1/p1q3pp/1p1r4/2p1pp1Q/1PPn1P2/3B3P/P2R2P1/3R2K1 b - - 0 1", "Rh6 e4",
    "4R3/4q1kp/6p1/1Q3b2/1P1b1P2/6KP/8/8 b - - 0 1", "Qh4+",
    "2b2rk1/p1p4p/2p1p1p1/br2N1Q1/1p2q3/8/PB3PPP/3R1RK1 w - - 0 1", "Nf7",
    "2k1rb1r/ppp3pp/2np1q2/5b2/2B2P2/2P1BQ2/PP1N1P1P/2KR3R b - - 0 1", "d5",
    "r4rk1/1bq1bp1p/4p1p1/p2p4/3BnP2/1N1B3R/PPP3PP/R2Q2K1 w - - 0 1", "Bxe4",
    "8/8/8/1p5r/p1p1k1pN/P2pBpP1/1P1K1P2/8 b - - 0 1", "Rxh4 b4",
    "2b5/1r6/2kBp1p1/p2pP1P1/2pP4/1pP3K1/1R3P2/8 b - - 0 1", "Rb4",
    "r4rk1/1b1nqp1p/p5p1/1p2PQ2/2p5/5N2/PP3PPP/R1BR2K1 w - - 0 1", "Bg5",
    "1R2rq1k/2p3p1/Q2p1pPp/8/4P3/8/P1r3PP/1R4K1 w - - 0 1", "Qb5 Rxe8",
    "5rk1/p1p2r1p/2pp2p1/4p3/PPPnP3/3Pq1P1/1Q1R1R1P/4NK2 b - - 0 1", "Nb3",
    "2kr1r2/p6p/5Pp1/2p5/1qp2Q1P/7R/PP6/1KR5 w - - 0 1", "Rb3",
    "5r2/1p1RRrk1/4Qq1p/1PP3p1/8/4B3/1b3P1P/6K1 w - - 0 1", "Qe4 Qxf7+ Rxf7+",
    "1R6/p5pk/4p2p/4P3/8/2r3qP/P3R1b1/4Q1K1 b - - 0 1", "Rc1",
    "r5k1/pQp2qpp/8/4pbN1/3P4/6P1/PPr4P/1K1R3R b - - 0 1", "Rc1+",
    "1k1r4/pp1r1pp1/4n1p1/2R5/2Pp1qP1/3P2QP/P4PB1/1R4K1 w - - 0 1", "Bxb7",
    "8/6k1/5pp1/Q6p/5P2/6PK/P4q1P/8 b - - 0 1", "Qf1+",
    "2b4k/p1b2p2/2p2q2/3p1PNp/3P2R1/3B4/P1Q2PKP/4r3 w - - 0 1", "Qxc6",
    "2rq1rk1/pp3ppp/2n2b2/4NR2/3P4/PB5Q/1P4PP/3R2K1 w - - 0 1", "Qxh7+",
    "r1b1r1k1/pp1nqp2/2p1p1pp/8/4N3/P1Q1P3/1P3PPP/1BRR2K1 w - - 0 1", "Rxd7",
    "1r3r1k/3p4/1p1Nn1R1/4Pp1q/pP3P1p/P7/5Q1P/6RK w - - 0 1", "Qe2",
    "r6r/pp3ppp/3k1b2/2pb4/B4Pq1/2P1Q3/P5PP/1RBR2K1 w - - 0 1", "Qxc5+",
    "4rrn1/ppq3bk/3pPnpp/2p5/2PB4/2NQ1RPB/PP5P/5R1K w - - 0 1", "Qxg6+",
    "6R1/4qp1p/ppr1n1pk/8/1P2P1QP/6N1/P4PP1/6K1 w - - 0 1", "Qh5+",
    "2k1r3/1p2Bq2/p2Qp3/Pb1p1p1P/2pP1P2/2P5/2P2KP1/1R6 w - - 0 1", "Rxb5",
    "5r1k/1p4pp/3q4/3Pp1R1/8/8/PP4PP/4Q1K1 b - - 0 1", "Qc5+",
    "r4rk1/pbq2pp1/1ppbpn1p/8/2PP4/1P1Q1N2/PBB2PPP/R3R1K1 w - - 0 1", "c5 d5",
    "1b5k/7P/p1p2np1/2P2p2/PP3P2/4RQ1R/q2r3P/6K1 w - - 0 1", "Re8+",
    "k7/p4p2/P1q1b1p1/3p3p/3Q4/7P/5PP1/1R4K1 w - - 0 1", "Qe5 Qf4",
    "1rb1r1k1/p1p2ppp/5n2/2pP4/5P2/2QB4/qNP3PP/2KRB2R b - - 0 1", "Bg4 Re2",
    "k5r1/p4b2/2P5/5p2/3P1P2/4QBrq/P5P1/4R1K1 w - - 0 1", "Qe8+",
    "r6k/pp3p1p/2p1bp1q/b3p3/4Pnr1/2PP2NP/PP1Q1PPN/R2B2RK b - - 0 1", "Nxh3",
    "3r3r/p4pk1/5Rp1/3q4/1p1P2RQ/5N2/P1P4P/2b4K w - - 0 1", "Rfxg6+",
    "3r1rk1/1pb1qp1p/2p3p1/p7/P2Np2R/1P5P/1BP2PP1/3Q1BK1 w - - 0 1", "Nf5",
    "4r1k1/pq3p1p/2p1r1p1/2Q1p3/3nN1P1/1P6/P1P2P1P/3RR1K1 w - - 0 1", "Rxd4",
    "r3brkn/1p5p/2p2Ppq/2Pp3B/3Pp2Q/4P1R1/6PP/5R1K w - - 0 1", "Bxg6",
    "r1bq1rk1/ppp2ppp/2np4/2bN1PN1/2B1P3/3p4/PPP2nPP/R1BQ1K1R w - - 0 1", "Qh5",
    "2r2b1r/p1Nk2pp/3p1p2/N2Qn3/4P3/q6P/P4PP1/1R3K1R w - - 0 1", "Qe6+",
    "r5k1/1bp3pp/p2p4/1p6/5p2/1PBP1nqP/1PP3Q1/R4R1K b - - 0 1", "Nd4",
    "6k1/p1B1b2p/2b3r1/2p5/4p3/1PP1N1Pq/P2R1P2/3Q2K1 b - - 0 1", "Rh6",
    "rnbqr2k/pppp1Qpp/8/b2NN3/2B1n3/8/PPPP1PPP/R1B1K2R w KQ - 0 1", "Qg8+",
    "r2r2k1/1R2qp2/p5pp/2P5/b1PN1b2/P7/1Q3PPP/1B1R2K1 b - - 0 1", "Qe5 Rab8",
    "2r1k2r/2pn1pp1/1p3n1p/p3PP2/4q2B/P1P5/2Q1N1PP/R4RK1 w k - 0 1", "exf6",
    "r3q2r/2p1k1p1/p5p1/1p2Nb2/1P2nB2/P7/2PNQbPP/R2R3K b - - 0 1", "Rxh2+",
    "2r1kb1r/pp3ppp/2n1b3/1q1N2B1/1P2Q3/8/P4PPP/3RK1NR w Kk - 0 1", "Nc7+",
    "2r3kr/ppp2n1p/7B/5q1N/1bp5/2Pp4/PP2RPPP/R2Q2K1 w - - 0 1", "Re8+",
    "2kr2nr/pp1n1ppp/2p1p3/q7/1b1P1B2/P1N2Q1P/1PP1BPP1/R3K2R w KQ - 0 1", "axb4",
    "2r1r1k1/pp1q1ppp/3p1b2/3P4/3Q4/5N2/PP2RPPP/4R1K1 w - - 0 1", "Qg4",
    "2kr4/ppp3Pp/4RP1B/2r5/5P2/1P6/P2p4/3K4 w - - 0 1", "Rd6",
    "nrq4r/2k1p3/1p1pPnp1/pRpP1p2/P1P2P2/2P1BB2/1R2Q1P1/6K1 w - - 0 1", "Bxc5",
    "2k4B/bpp1qp2/p1b5/7p/1PN1n1p1/2Pr4/P5PP/R3QR1K b - - 0 1", "Ng3+ g3",
    "8/1p6/p5R1/k7/Prpp4/K7/1NP5/8 w - - 0 1", "Rd6",
    "r1b2rk1/1p1n1ppp/p1p2q2/4p3/P1B1Pn2/1QN2N2/1P3PPP/3R1RK1 b - - 0 1", "Nc5 Nxg2 b5",
    "r5k1/pp1RR1pp/1b6/6r1/2p5/B6P/P4qPK/3Q4 w - - 0 1", "Qd5+",
    "1r4r1/p2kb2p/bq2p3/3p1p2/5P2/2BB3Q/PP4PP/3RKR2 b - - 0 1", "Rg3 Rxg2",
    "r2qkb1r/pppb2pp/2np1n2/5pN1/2BQP3/2N5/PPP2PPP/R1B1K2R w KQkq - 0 1", "Bf7+",
    "r7/4b3/2p1r1k1/1p1pPp1q/1P1P1P1p/PR2NRpP/2Q3K1/8 w - - 0 1", "Nxf5 Rc3",
    "r1r2bk1/5p1p/pn4p1/N2b4/3Pp3/B3P3/2q1BPPP/RQ3RK1 b - - 0 1", "Bxa3",
    "2R5/2R4p/5p1k/6n1/8/1P2QPPq/r7/6K1 w - - 0 1", "Rxh7+",
    "6k1/2p3p1/1p1p1nN1/1B1P4/4PK2/8/2r3b1/7R w - - 0 1", "Rh8+",
    "3q1rk1/4bp1p/1n2P2Q/3p1p2/6r1/Pp2R2N/1B4PP/7K w - - 0 1", "Ng5",
    "3r3k/pp4pp/8/1P6/3N4/Pn2P1qb/1B1Q2B1/2R3K1 w - - 0 1", "Nf5",
    "2rr3k/1b2bppP/p2p1n2/R7/3P4/1qB2P2/1P4Q1/1K5R w - - 0 1", "Qxg7+",
    "3r1k2/1p6/p4P2/2pP2Qb/8/1P1KB3/P6r/8 b - - 0 1", "Rxd5+",
    "rn3k1r/pp2bBpp/2p2n2/q5N1/3P4/1P6/P1P3PP/R1BQ1RK1 w - - 0 1", "Qg4 Qh5",
    "r1b2rk1/p4ppp/1p1Qp3/4P2N/1P6/8/P3qPPP/3R1RK1 w - - 0 1", "Nf6+",
    "2r3k1/5p1p/p3q1p1/2n3P1/1p1QP2P/1P4N1/PK6/2R5 b - - 0 1", "Qe5",
    "2k2r2/2p5/1pq5/p1p1n3/P1P2n1B/1R4Pp/2QR4/6K1 b - - 0 1", "Ne2+",
    "5r1k/3b2p1/p6p/1pRpR3/1P1P2q1/P4pP1/5QnP/1B4K1 w - - 0 1", "h3",
    "4r3/1Q1qk2p/p4pp1/3Pb3/P7/6PP/5P2/4R1K1 w - - 0 1", "d6+",
    "1nbq1r1k/3rbp1p/p1p1pp1Q/1p6/P1pPN3/5NP1/1P2PPBP/R4RK1 w - - 0 1", "Nfg5",
    "3r3k/1r3p1p/p1pB1p2/8/p1qNP1Q1/P6P/1P4P1/3R3K w - - 0 1", "Bf8 Nf5 Qf4",
    "4r3/p4r1p/R1p2pp1/1p1bk3/4pNPP/2P1K3/2P2P2/3R4 w - - 0 1", "Rxd5+",
    "3r4/1p2k2p/p1b1p1p1/4Q1Pn/2B3KP/4pP2/PP2R1N1/6q1 b - - 0 1", "Rd4+ Rf8",
    "3r1rk1/p3qp1p/2bb2p1/2p5/3P4/1P6/PBQN1PPP/2R2RK1 b - - 0 1", "Bxg2 Bxh2+",
    "3Q4/p3b1k1/2p2rPp/2q5/4B3/P2P4/7P/6RK w - - 0 1", "Qh8+",
    "1n2rr2/1pk3pp/pNn2p2/2N1p3/8/6P1/PP2PPKP/2RR4 w - - 0 1", "Nca4",
    "b2b1r1k/3R1ppp/4qP2/4p1PQ/4P3/5B2/4N1K1/8 w - - 0 1", "g6"
};

const int WinAtChessTestCount = sizeof(WinAtChessTests) / sizeof(char*) / 2;

void GeneratorTest1(void)
{
    U64 ResultNodes;

    char* ResultString;

    // Cache not used

    InitHashTable(1);
    ClearHash();

    // Threads not used

    omp_set_num_threads(1);

    for (int TestNumber = 0; TestNumber < GeneratorTestCount; ++TestNumber) {
        printf("\n");

        printf("Test %d\n", TestNumber + 1);
        printf("FEN = %s\n", GeneratorTests[TestNumber].Fen);

        SetFen(&CurrentBoard, GeneratorTests[TestNumber].Fen);

        PrintBoard(&CurrentBoard);

        printf("\n");

        for (int Depth = 1; Depth <= GeneratorTests[TestNumber].Depth; ++Depth) {
            ResultNodes = CountLegalMoves(&CurrentBoard, Depth);

            if (GeneratorTests[TestNumber].Nodes[Depth - 1] == ResultNodes) {
                ResultString = "PASSED";
            }
            else {
                ResultString = "FAILED";
            }

            printf("Depth = %2d Nodes = %10llu Result nodes = %10llu %s\n", Depth, GeneratorTests[TestNumber].Nodes[Depth - 1], ResultNodes, ResultString);
        }
    }
}

void GeneratorTest2(void)
{
    int InputDepth;

    U64 ResultNodes;

    // Cache not used

    InitHashTable(1);
    ClearHash();

    // Threads not used

    omp_set_num_threads(1);

    printf("\n");

    printf("Max. depth (0-%d): ", MAX_PLY);
    scanf_s("%d", &InputDepth);

    MaxDepth = (InputDepth >= 1 && InputDepth <= MAX_PLY) ? InputDepth : MAX_PLY;

    PrintBoard(&CurrentBoard);

    printf("\n");

    for (int Depth = 1; Depth <= MaxDepth; ++Depth) {
        ResultNodes = CountLegalMoves(&CurrentBoard, Depth);

        printf("Depth = %2d Result nodes = %10llu\n", Depth, ResultNodes);
    }
}

void Tests(const char* Tests[], const int TestCount)
{
    const char* Fen;
    const char* BestMoves;

    char NotateMoveStr[16];

    char* ResultString;

    int TestPassedCount = 0;

    InputParametrs();

    PrintMode = PRINT_MODE_TESTS;

    for (int TestNumber = 0; TestNumber < TestCount; ++TestNumber) {
        printf("\n");

        printf("Test %d\n", TestNumber + 1);

        Fen = Tests[TestNumber * 2];
        BestMoves = Tests[TestNumber * 2 + 1];

        printf("FEN = %s\n", Fen);
        printf("Best moves = %s\n", BestMoves);

        // Prepare new game

        SetFen(&CurrentBoard, Fen);

        ClearHash();

        PrintBoard(&CurrentBoard);

        ComputerMove();

        printf("\n");

        if (CurrentBoard.BestMovesRoot[0].Move) {
            NotateMove(&CurrentBoard, CurrentBoard.BestMovesRoot[0], NotateMoveStr);

            if (strstr(BestMoves, NotateMoveStr) != NULL) {
                ResultString = "PASSED";

                ++TestPassedCount;
            }
            else {
                ResultString = "FAILED";
            }

            printf("Best move: %s %s\n", NotateMoveStr, ResultString);
        }
        else { // No legal moves
            printf("No legal moves\n");
        }
    } // for

    PrintMode = PRINT_MODE_NORMAL;

    printf("\n");

    printf("Tests %d Tests passed %d (%d%%)\n", TestCount, TestPassedCount, 100 * TestPassedCount / TestCount);
}

void BratkoKopecTest(void)
{
    Tests(BratkoKopecTests, BratkoKopecTestCount);
}

void WinAtChessTest(void)
{
    Tests(WinAtChessTests, WinAtChessTestCount);
}

void SearchPerformanceTest(void)
{
    int MaxCycles;
    int MaxMoves;

    U64 TotalTestNodes = 0ULL;
    U64 TotalTestTime = 0ULL;

    InputParametrs();

    printf("\n");

    printf("Max. cycles: ");
    scanf_s("%d", &MaxCycles);

    MaxCycles = MAX(MaxCycles, 1);

    printf("Max. moves: ");
    scanf_s("%d", &MaxMoves);

    MaxMoves = MAX(MaxMoves, 1);

    for (int Cycle = 0; Cycle < MaxCycles; ++Cycle) {
        printf("\n");

        printf("Cycle = %d\n", Cycle + 1);

        // Prepare new game

        SetFen(&CurrentBoard, StartFen);

        ClearHash();

        PrintBoard(&CurrentBoard);

        for (int Move = 0; Move < MaxMoves * 2; ++Move) {
            if (!ComputerMove()) {
                return;
            }

            TotalTestNodes += CurrentBoard.Nodes;
            TotalTestTime += TotalTime;
        }
    }

    printf("\n");

    printf("Total nodes %llu Total time %.2f\n", TotalTestNodes, (double)TotalTestTime / 1000.0);

    printf("Average nodes %llu Average time %.2f\n", TotalTestNodes / (U64)MaxCycles, (double)TotalTestTime / (double)MaxCycles / 1000.0);

    if (TotalTestTime >= 1000ULL) {
        printf("NPS %llu\n", 1000ULL * TotalTestNodes / TotalTestTime);
    }
}

void EvaluateTests(const char* Tests[], const int TestCount)
{
    int MaxCycles;

    const char* Fen;

    LARGE_INTEGER EvaluateTimeStart;
    LARGE_INTEGER EvaluateTimeStop;

    LARGE_INTEGER Frequency;

    I64 EvaluateTotalTime = 0LL;

    printf("\n");

    printf("Max. cycles: ");
    scanf_s("%d", &MaxCycles);

    MaxCycles = MAX(MaxCycles, 1);

    printf("\n");

    for (int Cycle = 0; Cycle < MaxCycles; ++Cycle) {
        printf("Cycle = %d\n", Cycle + 1);

        for (int TestNumber = 0; TestNumber < TestCount; ++TestNumber) {
            Fen = Tests[TestNumber * 2];

            SetFen(&CurrentBoard, Fen);

            QueryPerformanceCounter(&EvaluateTimeStart);

            Evaluate(&CurrentBoard);

            QueryPerformanceCounter(&EvaluateTimeStop);

            EvaluateTotalTime += EvaluateTimeStop.QuadPart - EvaluateTimeStart.QuadPart;
        }
    }

    QueryPerformanceFrequency(&Frequency);

    // We now have the elapsed number of ticks, along with the number of ticks-per-second.
    // We use these values to convert to the number of elapsed microseconds.
    // To guard against loss-of-precision, we convert to microseconds before dividing by ticks-per-second.
    EvaluateTotalTime *= 1000000LL;
    EvaluateTotalTime /= Frequency.QuadPart;

    printf("\n");

    printf("EPS %lld\n", 1000000LL * (I64)TestCount * (I64)MaxCycles / EvaluateTotalTime);
}

void EvaluatePerformanceTest(void)
{
//    EvaluateTests(BratkoKopecTests, BratkoKopecTestCount);
    EvaluateTests(WinAtChessTests, WinAtChessTestCount);
}