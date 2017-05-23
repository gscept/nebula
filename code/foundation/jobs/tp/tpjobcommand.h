#pragma once
//------------------------------------------------------------------------------
/**
    @class Jobs::TPJobCommand
    
    A command queue entry for the worker threads.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "jobs/tp/tpjobslice.h"

//------------------------------------------------------------------------------
namespace Jobs
{
class TPJobCommand
{
public:
    /// commands
    enum Code
    {
        Run,        // run job slices
        Sync,       // wait on an event
        
        InvalidCode,
    };

    /// constructor
    TPJobCommand();

    /// setup for sync command
    void SetupSync(const Threading::Event* syncEvent);
    /// setup for run job slices command
    void SetupRun(TPJobSlice* firstSlice, ushort numSlices, ushort stride);

    /// get the command code
    Code GetCode() const;
    /// get pointer to sync event object
    const Threading::Event* GetSyncEvent() const;
    /// get pointer to first job slice
    TPJobSlice* GetFirstSlice() const;
    /// get number of slices
    ushort GetNumSlices() const;
    /// get slice array stride
    ushort GetStride() const;
    
private:    
    Code code;
    union
    {
        const Threading::Event* syncEvent;
        TPJobSlice* firstSlice;
    };
    ushort numSlices;
    ushort stride;
};

//------------------------------------------------------------------------------
/**
*/
inline
TPJobCommand::TPJobCommand() :
    firstSlice(0),
    numSlices(0),
    stride(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline void
TPJobCommand::SetupSync(const Threading::Event* syncEvent_)
{
    this->code = Sync;
    this->syncEvent = syncEvent_;
    this->numSlices = 0;
    this->stride = 0;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TPJobCommand::SetupRun(TPJobSlice* firstSlice_, ushort numSlices_, ushort stride_)
{
    this->code = Run;
    this->firstSlice = firstSlice_;
    this->numSlices = numSlices_;
    this->stride = stride_;
}

//------------------------------------------------------------------------------
/**
*/
inline TPJobCommand::Code 
TPJobCommand::GetCode() const
{
    return this->code;
}

//------------------------------------------------------------------------------
/**
*/
inline const Threading::Event*
TPJobCommand::GetSyncEvent() const
{
    n_assert(Sync == this->code);
    return this->syncEvent;
}

//------------------------------------------------------------------------------
/**
*/
inline TPJobSlice*
TPJobCommand::GetFirstSlice() const
{
    n_assert(Run == this->code);
    return this->firstSlice;
}

//------------------------------------------------------------------------------
/**
*/
inline ushort
TPJobCommand::GetNumSlices() const
{
    return this->numSlices;
}

//------------------------------------------------------------------------------
/**
*/
inline ushort
TPJobCommand::GetStride() const
{
    return this->stride;
}

} // namespace Jobs
//------------------------------------------------------------------------------
    