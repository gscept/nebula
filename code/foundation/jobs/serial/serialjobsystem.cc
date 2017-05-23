//------------------------------------------------------------------------------
//  serialjobsystem.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "jobs/serial/serialjobsystem.h"

namespace Jobs
{
__ImplementClass(Jobs::SerialJobSystem, 'SLJS', Base::JobSystemBase);

//------------------------------------------------------------------------------
/**
*/
SerialJobSystem::SerialJobSystem() :
    scratchBuffer(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
SerialJobSystem::~SerialJobSystem()
{
    n_assert(0 == this->scratchBuffer);
}    

//------------------------------------------------------------------------------
/**
*/
void
SerialJobSystem::Setup()
{
    n_assert(0 == this->scratchBuffer);
    JobSystemBase::Setup();
    this->scratchBuffer = Memory::Alloc(Memory::ScratchHeap, MaxScratchSize);
}

//------------------------------------------------------------------------------
/**
*/
void
SerialJobSystem::Discard()
{
    n_assert(0 != this->scratchBuffer);
    Memory::Free(Memory::ScratchHeap, this->scratchBuffer);
    this->scratchBuffer = 0;
    JobSystemBase::Discard();
}

} // namespace Jobs