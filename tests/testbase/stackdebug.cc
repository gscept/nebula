//------------------------------------------------------------------------------
//  stackdebug.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"

#include "stackdebug.h"

#if __PS3__

#include "core/debug.h"

using namespace Test;

static const int CHECKPOINTS_MAX = 40;
static const int MESSAGE_LEN_MAX = 50;
struct Checkpoint
{
    char msg[MESSAGE_LEN_MAX];
    volatile unsigned int *stack_ptr;
};
static Checkpoint checkpoints[CHECKPOINTS_MAX];
static int checkpointCount = 0;

void Test::StackCheckpoint(const char *msg, volatile unsigned int *stack_ptr)
{
    n_assert(checkpointCount < CHECKPOINTS_MAX);
    Checkpoint *ck = &checkpoints[checkpointCount++];
    strncpy(ck->msg, msg, MESSAGE_LEN_MAX);
    ck->msg[MESSAGE_LEN_MAX - 1] = '\0';
    ck->stack_ptr = stack_ptr;
}

void Test::DumpStackCheckpoints()
{
    n_printf("\n\nStack Checkpoints:\n\n");
    int i;
    for(i = 0; i < checkpointCount; ++i)
    {
        n_printf("\t[%-35s] stack ptr: 0x%p\n", checkpoints[i].msg, checkpoints[i].stack_ptr);
    }
}

#endif