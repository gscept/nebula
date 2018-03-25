//------------------------------------------------------------------------------
//  jobsystembase.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "jobs/base/jobsystembase.h"

namespace Base
{
__ImplementClass(Base::JobSystemBase, 'JBSB', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
JobSystemBase::JobSystemBase() :
    isValid(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
JobSystemBase::~JobSystemBase()
{
    n_assert(!this->IsValid());
}

//------------------------------------------------------------------------------
/**
*/
void
JobSystemBase::Setup()
{
    n_assert(!this->IsValid());
    this->isValid = true;
}

//------------------------------------------------------------------------------
/**
*/
void
JobSystemBase::Discard()
{
    n_assert(this->IsValid());
    this->isValid = false;
}

} // namespace Base
