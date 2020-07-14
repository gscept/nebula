#pragma once

//------------------------------------------------------------------------------
/**
*/

#if __PS3__
register volatile unsigned int *g_stack_ptr asm("1");
// is a macro, so there is no additional local stack frame, when g_stack_ptr is being evaluated
#define STACK_CHECKPOINT(msg) StackCheckpoint(msg, g_stack_ptr)
#define DUMP_STACK_CHECKPOINTS DumpStackCheckpoints()

namespace Test
{
    void StackCheckpoint(const char *msg, volatile unsigned int *stack_ptr);
    void DumpStackCheckpoints();
}
#else
#define STACK_CHECKPOINT(msg) 
#define DUMP_STACK_CHECKPOINTS 
#endif
