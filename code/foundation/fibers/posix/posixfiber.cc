//------------------------------------------------------------------------------
//  posixfiber.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "fibers/fiber.h"
#include "memory/memory.h"
#include "core/debug.h"
#include <ucontext.h>
#include <setjmp.h>

namespace Fibers
{


struct fiber_t
{
    ucontext_t  fib;
    jmp_buf     jmp;
};

struct fiber_ctx_t
{
    void(*fnc)(void*);
    void* ctx;
    jmp_buf* cur;
    ucontext_t* prv;
};

//------------------------------------------------------------------------------
/**
*/
Fiber::Fiber()
    : handle(nullptr)
    , context(nullptr)
{
}

//------------------------------------------------------------------------------
/**
*/
Fiber::Fiber(std::nullptr_t)
    : handle(nullptr)
    , context(nullptr)
{
}

//------------------------------------------------------------------------------
/**
*/
static void FiberStartFunction(void* p)
{
    fiber_ctx_t* ctx = (fiber_ctx_t*)p;
    void (*ufnc)(void*) = ctx->fnc;
    void* uctx = ctx->ctx;
    if (_setjmp(*ctx->cur) == 0)
    {
        ucontext_t tmp;
        swapcontext(&tmp, ctx->prv);
    }
    ufnc(uctx);
}

//------------------------------------------------------------------------------
/**
*/
Fiber::Fiber(void(*function)(void*), void* context)
    : handle(nullptr)
    , context(nullptr)
{
    this->handle = new fiber_t;
    fiber_t* implHandle = (fiber_t*)this->handle;
    getcontext(&implHandle->fib);

    const size_t stackSize = 64 * 1024;
    implHandle->fib.uc_stack.ss_sp = new char[stackSize];
    implHandle->fib.uc_stack.ss_size = stackSize;
    implHandle->fib.uc_link = nullptr;

    ucontext_t temp;
    fiber_ctx_t ctx = { function, context, &implHandle->jmp, &temp };
    makecontext(&implHandle->fib, (void(*)())FiberStartFunction, 1, &ctx);
    swapcontext(&temp, &implHandle->fib);
    this->context = context;
}

//------------------------------------------------------------------------------
/**
*/
Fiber::Fiber(const Fiber& rhs)
{
    if (this->handle != nullptr)
    {
        fiber_t* implHandle = (fiber_t*)this->handle;
        delete[] (char*)implHandle->fib.uc_stack.ss_sp;
    }
    this->handle = rhs.handle;
    this->context = rhs.context;
}

//------------------------------------------------------------------------------
/**
*/
Fiber::~Fiber()
{
    if (this->handle != nullptr)
    {
        fiber_t* implHandle = (fiber_t*)this->handle;
        delete[] (char*)implHandle->fib.uc_stack.ss_sp;
    }
    this->handle = nullptr;
    this->context = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
Fiber::operator=(const Fiber& rhs)
{
    if (this->handle != nullptr)
    {
        fiber_t* implHandle = (fiber_t*)this->handle;
        delete[] (char*)implHandle->fib.uc_stack.ss_sp;
    }
    this->handle = rhs.handle;
    this->context = rhs.context;
}

//------------------------------------------------------------------------------
/**
*/
void
Fiber::ThreadToFiber(Fiber& fiber)
{
    fiber.handle = Memory::Alloc(Memory::AppHeap, sizeof(fiber_t));
    Memory::Clear(fiber.handle, sizeof(fiber_t));
}

//------------------------------------------------------------------------------
/**
*/
void
Fiber::FiberToThread(Fiber& fiber)
{
    Memory::Free(Memory::AppHeap, fiber.handle);
    fiber.handle = nullptr;
    fiber.context = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
Fiber::SwitchToFiber(Fiber& CurrentFiber)
{
    n_assert(this->handle != nullptr);
    fiber_t* implHandle = (fiber_t*)this->handle;
    fiber_t* currHandle = (fiber_t*)CurrentFiber.handle;
    if (_setjmp(currHandle->jmp) == 0)
    {
        _longjmp(implHandle->jmp, 1);
    }
}

}
