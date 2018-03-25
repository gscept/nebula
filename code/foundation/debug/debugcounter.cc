//------------------------------------------------------------------------------
//  debugcounter.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "debug/debugcounter.h"
#include "debug/debugserver.h"

namespace Debug
{
__ImplementClass(Debug::DebugCounter, 'DBGC', Core::RefCounted);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
DebugCounter::DebugCounter() :
    value(0),
    history(1024)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
DebugCounter::~DebugCounter()
{
    // we may need de-construct if the critical section is still taken
    this->critSect.Enter();
    n_assert(!this->IsValid());
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
DebugCounter::Setup(const StringAtom& counterName, const Util::StringAtom& group)
{
    n_assert(counterName.IsValid());
    n_assert(!this->IsValid());

    this->critSect.Enter();
    this->name = counterName;
	this->group = group;
    this->history.Reset();
    this->value = 0;
    this->critSect.Leave();

    DebugServer::Instance()->RegisterDebugCounter(this);
}

//------------------------------------------------------------------------------
/**
*/
void
DebugCounter::Discard()
{
    n_assert(this->IsValid());
    DebugServer::Instance()->UnregisterDebugCounter(this);

    this->critSect.Enter();
    this->history.Reset();
    this->value = 0;
    this->name.Clear();
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
int
DebugCounter::GetSample() const
{
    int val = 0;
    this->critSect.Enter();
    if (!this->history.IsEmpty())
    {
        val = this->history.Back();
    }
    this->critSect.Leave();
    return val;
}

//------------------------------------------------------------------------------
/**
*/
Array<int>
DebugCounter::GetHistory() const
{
    this->critSect.Enter();
    Array<int> result = this->history.AsArray();
    this->critSect.Leave();
    return result;
}

} // namespace Debug