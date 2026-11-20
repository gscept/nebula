//------------------------------------------------------------------------------
//  win32fiber.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "fibers/fiber.h"
#include "core/debug.h"

namespace Fibers
{

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
Fiber::Fiber(void(*function)(void*), void* context)
    : handle(nullptr)
    , context(nullptr)
{
    // FIXME: Fibers shouldn't actually own the fiber handle, instead should be 
    //        fetched from the pool
    this->handle = CreateFiber(0, { function }, context);
    this->context = context;
}

//------------------------------------------------------------------------------
/**
*/
Fiber::Fiber(const Fiber& rhs)
{
    // BUG: We're moving/sharing ownership here potentially and the destructor of
    //      one fiber might destroy it for the other, leading to double-free
    this->handle = rhs.handle;
    this->context = rhs.context;
}

//------------------------------------------------------------------------------
/**
*/
Fiber::~Fiber()
{
    // BUG: We might not own this handle!
    if (this->handle != nullptr)
        DeleteFiber(this->handle);
    this->handle = nullptr;
    this->context = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
Fiber::operator=(const Fiber& rhs)
{
    // BUG: Might not own this handle!
    if (this->handle != nullptr)
        DeleteFiber(this->handle);
    this->handle = rhs.handle;
    this->context = rhs.context;
}

//------------------------------------------------------------------------------
/**
*/
void
Fiber::ThreadToFiber(Fiber& fiber)
{
    fiber.handle = ConvertThreadToFiber(nullptr);
    fiber.context = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
Fiber::FiberToThread(Fiber& fiber)
{
    n_assert(ConvertFiberToThread());
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
    ::SwitchToFiber(this->handle);
}

}
