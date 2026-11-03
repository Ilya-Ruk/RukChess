// Utils.cpp

#include "stdafx.h"

#include "Utils.h"

#include "Board.h"
#include "Def.h"
#include "Game.h"
#include "Types.h"

#ifdef BIND_THREAD_V1
int ThreadNode[MAX_THREADS];
#endif // BIND_THREAD_V1

U64 RandState = 0ULL; // The state can be seeded with any value

/*
    Time in milliseconds since midnight (00:00:00), January 1, 1970, coordinated universal time (UTC)
*/
U64 Clock(void)
{
    struct _timeb timebuffer;

    _ftime_s(&timebuffer);

    return timebuffer.time * 1000ULL + (U64)timebuffer.millitm;
}

/*
    https://prng.di.unimi.it/splitmix64.c
*/
U64 Rand64(void)
{
    U64 Result = (RandState += 0x9E3779B97F4A7C15);

    Result = (Result ^ (Result >> 30)) * 0xBF58476D1CE4E5B9;
    Result = (Result ^ (Result >> 27)) * 0x94D049BB133111EB;

    return Result ^ (Result >> 31);
}

void SetRandState(const U64 NewRandState)
{
    RandState = NewRandState;
}

#ifdef BIND_THREAD_V1

void InitThreadNode(void)
{
    int Nodes = 0;      // Physical processors
    int Cores = 0;      // Logical processors
    int Threads = 0;    // Threads

    int Index = 0;

    DWORD ReturnedLength = 0;
    DWORD ByteOffset = 0;

    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* Buffer;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* Ptr;

    // First call to get ReturnLength. We expect it to fail due to null buffer
    if (GetLogicalProcessorInformationEx(RelationAll, NULL, &ReturnedLength)) {
        printf("Get logical processor information error!\n");

        return;
    }

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        printf("Get logical processor information error!\n");

        return;
    }

    // Once we know ReturnLength, allocate the buffer
    Buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)malloc(ReturnedLength);

    if (Buffer == NULL) { // Allocate memory error
        printf("Allocate memory to buffer error!\n");

        return;
    }

    // Second call, now we expect to succeed
    if (!GetLogicalProcessorInformationEx(RelationAll, Buffer, &ReturnedLength)) {
        printf("Get logical processor information error!\n");

        free(Buffer);

        return;
    }

    Ptr = Buffer;

    while (ByteOffset < ReturnedLength) {
        if (Ptr->Relationship == RelationNumaNode) {
            ++Nodes;
        }
        else if (Ptr->Relationship == RelationProcessorCore) {
            ++Cores;

            Threads += (Ptr->Processor.Flags == LTP_PC_SMT) ? 2 : 1;
        }

        ByteOffset += Ptr->Size;

        Ptr = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)((char*)Ptr + Ptr->Size);
    }

    free(Buffer);

    // Run as many threads as possible on the same node until core limit is reached,
    // then move on filling the next node
    for (int Node = 0; Node < Nodes; ++Node) {
        for (int Core = 0; Core < Cores / Nodes; ++Core) {
            ThreadNode[Index++] = Node;
        }
    }

    // In case a core has more than one logical processor (we assume 2) and
    // we have still threads to allocate, then spread them evenly across available nodes
    for (int Thread = 0; Thread < Threads - Cores; ++Thread) {
        ThreadNode[Index++] = Thread % Nodes;
    }
}

void BindThread(const int ThreadNumber)
{
    USHORT Node;

    GROUP_AFFINITY GroupAffinity;

    if (ThreadNumber < 0 || ThreadNumber >= MaxThreads) {
        return;
    }

    Node = (USHORT)ThreadNode[ThreadNumber];

    if (GetNumaNodeProcessorMaskEx(Node, &GroupAffinity)) {
        SetThreadGroupAffinity(GetCurrentThread(), &GroupAffinity, NULL);
    }
}

#endif // BIND_THREAD_V1

#ifdef BIND_THREAD_V2
void BindThread(const int ThreadNumber)
{
    DWORD_PTR AffinityMask;

    if (ThreadNumber < 0 || ThreadNumber >= MIN(MaxThreads, 64)) { // Max. 64 CPUs [0..63]
        return;
    }

    AffinityMask = 1ULL << ThreadNumber;

    SetThreadAffinityMask(GetCurrentThread(), AffinityMask);
}
#endif // BIND_THREAD_V2