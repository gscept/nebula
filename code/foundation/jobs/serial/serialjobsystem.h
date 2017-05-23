#pragma once
//------------------------------------------------------------------------------
/**
    @class Jobs::SerialJobSystem
    
    A dummy job system which processes jobs in the caller thread. This is
    useful for single-core systems like the Wii.
    
    (C) 2009 Radon Labs GmbH
*/
#include "jobs/base/jobsystembase.h"

//------------------------------------------------------------------------------
namespace Jobs
{
class SerialJobSystem : public Base::JobSystemBase
{
    __DeclareClass(SerialJobSystem);
public:
    /// constructor
    SerialJobSystem();
    /// destructor
    virtual ~SerialJobSystem();    

    /// setup the job system
    void Setup();
    /// shutdown the job system
    void Discard();

private:
    friend class SerialJobPort;
    static const SizeT MaxScratchSize = (64 * 1024);

    /// get pointer to scratch buffer
    void* GetScratchBuffer() const;

    void* scratchBuffer;
};

//------------------------------------------------------------------------------
/**
*/
inline void*
SerialJobSystem::GetScratchBuffer() const
{
    return this->scratchBuffer;
}

} // namespace Jobs
//------------------------------------------------------------------------------
