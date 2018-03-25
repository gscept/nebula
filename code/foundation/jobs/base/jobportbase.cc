//------------------------------------------------------------------------------
//  jobportbase.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "jobs/base/jobportbase.h"

namespace Base
{
__ImplementClass(Base::JobPortBase, 'JBPB', Core::RefCounted);

using namespace Jobs;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
JobPortBase::JobPortBase() :
    isValid(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
JobPortBase::~JobPortBase()
{
    n_assert(!this->IsValid());
}

//------------------------------------------------------------------------------
/**
*/
void
JobPortBase::Setup()
{
    n_assert(!this->IsValid());
    this->isValid = true;
}

//------------------------------------------------------------------------------
/**
*/
void
JobPortBase::Discard()
{
    n_assert(this->IsValid());
    this->isValid = false;
}

//------------------------------------------------------------------------------
/**
*/
void
JobPortBase::PushJob(const Ptr<Job>& job)
{
    (void)job;  // shutoff unused parameter warning
    // override in subclass!
}

//------------------------------------------------------------------------------
/**
*/
void
JobPortBase::PushJobChain(const Array<Ptr<Job> >& jobs)
{
    (void)jobs; // shutoff unused parameter warning
    // override in subclass!
}

//------------------------------------------------------------------------------
/**
*/
void
JobPortBase::PushFlush()
{
    // override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
JobPortBase::PushSync()
{
    // override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
JobPortBase::WaitDone()
{
    // override in subclass
}

//------------------------------------------------------------------------------
/**
*/
bool
JobPortBase::CheckDone()
{
    // override in subclass!
    return false;
}

} // namespace Base