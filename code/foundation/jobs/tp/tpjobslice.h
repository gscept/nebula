#pragma once
//------------------------------------------------------------------------------
/**
    @class Jobs::TPJobSlice
  
    A "mini job" which works on a single job slice.

    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Jobs
{
class TPJob;

class TPJobSlice
{
public:
    /// constructor
    TPJobSlice();
    /// destructor
    ~TPJobSlice();
    
    /// setup the job slice 
    void Setup(TPJob* job, IndexT sliceIndex);
    /// get pointer to job
    TPJob* GetJob() const;
    /// get slice index
    IndexT GetSliceIndex() const;

private:
    TPJob* job;
    IndexT sliceIndex;
};

//------------------------------------------------------------------------------
/**
*/
inline TPJob*
TPJobSlice::GetJob() const
{
    return this->job;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
TPJobSlice::GetSliceIndex() const
{
    return this->sliceIndex;
}

} // namespace Jobs
//------------------------------------------------------------------------------
