//------------------------------------------------------------------------------
//  tpjobsystem.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "jobs/tp/tpjobsystem.h"

namespace Jobs
{
__ImplementClass(Jobs::TPJobSystem, 'TPJS', Base::JobSystemBase);

using namespace Base;

//------------------------------------------------------------------------------
/**
*/
TPJobSystem::TPJobSystem()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
TPJobSystem::~TPJobSystem()
{
    if (this->IsValid())
    {
        this->Discard();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TPJobSystem::Setup()
{
    n_assert(!this->IsValid());
    JobSystemBase::Setup();
    this->threadPool.Setup();
}

//------------------------------------------------------------------------------
/**
*/
void
TPJobSystem::Discard()
{
    n_assert(this->IsValid());
    this->threadPool.Discard();
    JobSystemBase::Discard();
}

} // namespace Jobs
