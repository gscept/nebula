//------------------------------------------------------------------------------
//  win32fiber.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "fibers/fiber.h"
#include "core/debug.h"
namespace Win32
{

} // namespace Win32

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
	this->handle = CreateFiber(0, { function }, context);
	this->context = context;
}

//------------------------------------------------------------------------------
/**
*/
Fiber::Fiber(const Fiber& rhs)
{
	if (this->handle != nullptr)
		DeleteFiber(this->handle);
	this->handle = rhs.handle;
	this->context = rhs.context;
}

//------------------------------------------------------------------------------
/**
*/
Fiber::~Fiber()
{
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
Fiber::Start()
{
	n_assert(this->handle != nullptr);
	SwitchToFiber(this->handle);
}

}
