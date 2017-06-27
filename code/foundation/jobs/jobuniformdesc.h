#pragma once
//------------------------------------------------------------------------------
/**
    @class Jobs::JobUniformDesc
    
    Descriptor for uniform data of a Job.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Jobs
{
class JobUniformDesc
{
public:
    static const SizeT MaxNumBuffers = 4;

    /// default constructor
    JobUniformDesc();
	/// construct from nullptr
	JobUniformDesc(nullptr_t);
	/// constructor with array of pointers, but may not exceed MaxNumBuffers
	JobUniformDesc(std::initializer_list<std::tuple<void*, SizeT, SizeT>> data, SizeT scratchSize);
    /// constructor with 1 uniform buffer
    JobUniformDesc(void* ptr, SizeT bufSize, SizeT scratchSize);
    /// constructor with 2 uniform buffers
    JobUniformDesc(void* ptr0, SizeT bufSize0, void* ptr1, SizeT bufSize1, SizeT scratchSize);
	/// constructor with 3 uniform buffers
	JobUniformDesc(void* ptr0, SizeT bufSize0, void* ptr1, SizeT bufSize1, void* ptr2, SizeT bufSize2, SizeT scratchSize);

    /// update the uniform desc, all sizes in bytes
    void Update(IndexT index, void* ptr, SizeT bufSize);
	/// update only scratch size
	void Update(SizeT scratchSize);

    /// get number of buffers
    SizeT GetNumBuffers() const;
    /// get buffer pointer
    void* GetPointer(IndexT i) const;
    /// get buffer size
    SizeT GetBufferSize(IndexT i) const;
    /// get scratch size
    SizeT GetScratchSize() const;

private:
    SizeT numBuffers;
    void* ptr[MaxNumBuffers];
    SizeT bufferSize[MaxNumBuffers];
    SizeT scratchSize;
};

//------------------------------------------------------------------------------
/**
*/
inline
JobUniformDesc::JobUniformDesc() :
    numBuffers(0),
    scratchSize(0)
{
    IndexT i;
    for (i = 0; i < MaxNumBuffers; i++)
    {
        this->ptr[i] = 0;
        this->bufferSize[i] = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline 
JobUniformDesc::JobUniformDesc(nullptr_t)
{
	IndexT i;
	for (i = 0; i < MaxNumBuffers; i++)
	{
		this->ptr[i] = 0;
		this->bufferSize[i] = 0;
	}
}

//------------------------------------------------------------------------------
/**
*/
inline
JobUniformDesc::JobUniformDesc(std::initializer_list<std::tuple<void*, SizeT, SizeT>> data, SizeT scratchSize)
{
	n_assert(data.size() <= MaxNumBuffers, "Too many data points, refer to MaxNumBuffers");
	this->numBuffers = data.size();
	this->scratchSize = scratchSize;

	IndexT i;
	for (i = 0; i < data.size(); i++)
	{
		const std::tuple<void*, SizeT, SizeT>& d = data.begin()[i];
		this->ptr[i] = std::get<0>(d);
		this->bufferSize[i] = std::get<1>(d);
	}
}

//------------------------------------------------------------------------------
/**
*/
inline
JobUniformDesc::JobUniformDesc(void* ptr_, SizeT bufSize_, SizeT scratchSize_) :
    numBuffers(1)
{
    this->ptr[0] = ptr_;
    this->bufferSize[0] = bufSize_;
    this->ptr[1] = 0;
    this->bufferSize[1] = 0;
    this->scratchSize = scratchSize_;
}

//------------------------------------------------------------------------------
/**
*/
inline
JobUniformDesc::JobUniformDesc(void* ptr0_, SizeT bufSize0_, void* ptr1_, SizeT bufSize1_, SizeT scratchSize_) :
    numBuffers(2)
{
    this->ptr[0] = ptr0_;
    this->bufferSize[0] = bufSize0_;
    this->ptr[1] = ptr1_;
    this->bufferSize[1] = bufSize1_;
    this->scratchSize = scratchSize_;
}

//------------------------------------------------------------------------------
/**
*/
inline
JobUniformDesc::JobUniformDesc(void* ptr0_, SizeT bufSize0_, void* ptr1_, SizeT bufSize1_, void* ptr2_, SizeT bufSize2_, SizeT scratchSize_) :
	numBuffers(3)
{
	this->ptr[0] = ptr0_;
	this->bufferSize[0] = bufSize0_;
	this->ptr[1] = ptr1_;
	this->bufferSize[1] = bufSize1_;
	this->ptr[2] = ptr2_;
	this->bufferSize[2] = bufSize2_;
	this->scratchSize = scratchSize_;
}

//------------------------------------------------------------------------------
/**
*/
inline void
JobUniformDesc::Update(IndexT index, void* ptr_, SizeT size_)
{
    n_assert(index < this->numBuffers);
    this->ptr[index] = ptr_;
    this->bufferSize[index] = size_;
}

//------------------------------------------------------------------------------
/**
*/
inline void
JobUniformDesc::Update(SizeT scratchSize_)
{
	this->scratchSize = scratchSize_;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
JobUniformDesc::GetNumBuffers() const
{
    return this->numBuffers;
}

//------------------------------------------------------------------------------
/**
*/
inline void*
JobUniformDesc::GetPointer(IndexT i) const
{
    n_assert(i < MaxNumBuffers);
    return this->ptr[i];
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
JobUniformDesc::GetBufferSize(IndexT i) const
{
    n_assert(i < MaxNumBuffers);
    return this->bufferSize[i];
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT 
JobUniformDesc::GetScratchSize() const
{
    return this->scratchSize;
}

} // namespace Jobs
//------------------------------------------------------------------------------
   