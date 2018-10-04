#pragma once
//------------------------------------------------------------------------------
/**
    @class Debug::DebugCounter
  
    A debug counter for counting events.
      
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "util/stringatom.h"
#include "util/ringbuffer.h"
#include "threading/criticalsection.h"

//------------------------------------------------------------------------------
#if NEBULA_ENABLE_PROFILING
#define _declare_counter(counter) Ptr<Debug::DebugCounter> counter;
#define _setup_counter(counter) counter = Debug::DebugCounter::Create(); counter->Setup(Util::StringAtom(#counter));
#define _setup_grouped_counter(counter, group) counter = Debug::DebugCounter::Create(); counter->Setup(Util::StringAtom(#counter), group);
#define _discard_counter(counter) counter->Discard(); counter = nullptr;
#define _begin_counter(counter) counter->Begin();
#define _begin_counter_noreset(counter) counter->Begin(false);
#define _reset_counter(counter) counter->Reset();
#define _incr_counter(counter, val) counter->Incr(val);
#define _decr_counter(counter, val) counter->Decr(val);
#define _set_counter(counter, val) counter->Set(val);
#define _end_counter(counter) counter->End();
#else
#define _declare_counter(counter)
#define _setup_counter(counter)
#define _setup_grouped_counter(counter, group)
#define _discard_counter(counter)
#define _begin_counter(counter)
#define _begin_counter_noreset()
#define _reset_counter(counter)
#define _incr_counter(counter, val)
#define _decr_counter(counter, val)
#define _set_counter(counter, val)
#define _end_counter(counter)
#endif
//------------------------------------------------------------------------------
namespace Debug
{
class DebugCounter : public Core::RefCounted
{
    __DeclareClass(DebugCounter);
public:
    /// constructor
    DebugCounter();
    /// destructor
    virtual ~DebugCounter();

    /// setup the counter
	void Setup(const Util::StringAtom& name, const Util::StringAtom& group = Util::StringAtom("Ungrouped"));
    /// discard the counter
    void Discard();
    /// return true if object has been setup
    bool IsValid() const;
    
    /// begin counting, optionally reset the counter
    void Begin(bool reset=true);
    /// manually reset the counter to zero
    void Reset();
    /// increment the counter by a specific value
    void Incr(int amount);
    /// decrement the counter by a specific value
    void Decr(int amount);
    /// set the counter directly
    void Set(int val);
    /// end counting, write current value to history
    void End();
    
    /// get the counter's name
    const Util::StringAtom& GetName() const;
	/// get the timer group
	const Util::StringAtom& GetGroup() const;
    /// get the most recent sample
    int GetSample() const;
    /// get the counter's history
    Util::Array<int> GetHistory() const;

private:
    Threading::CriticalSection critSect;
    Util::StringAtom name;
	Util::StringAtom group;
    int value;
    Util::RingBuffer<int> history;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
DebugCounter::IsValid() const
{
    return this->name.IsValid();
}

//------------------------------------------------------------------------------
/**
*/
inline void
DebugCounter::Begin(bool reset)
{
    if (reset)
    {
        this->value = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
DebugCounter::Reset()
{
    this->value = 0;
}

//------------------------------------------------------------------------------
/**
*/
inline void
DebugCounter::Incr(int amount)
{
    this->value += amount;
}

//------------------------------------------------------------------------------
/**
*/
inline void
DebugCounter::Decr(int amount)
{
    this->value -= amount;
}

//------------------------------------------------------------------------------
/**
*/
inline void
DebugCounter::Set(int val)
{
    this->value = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
DebugCounter::End()
{
    this->critSect.Enter();
    this->history.Add(this->value);
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
DebugCounter::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
DebugCounter::GetGroup() const
{
	return this->group;
}

} // namespace Debug
//------------------------------------------------------------------------------
