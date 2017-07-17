#pragma once
//------------------------------------------------------------------------------
/**
    @class Jobs::JobDataDesc
    
    Descriptor for input/output data of a job. Input/output data is
    split into elements and slices. A job function may be called
    with any number of elements, up to the MaxElementsPerSlice number.
    Within a current slice, the job may perform random access on
    elements. Slices may not depend on each other (the job system
    may split a job into slices which are processed in parallel).
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/

//------------------------------------------------------------------------------
#include <tuple>
#include <initializer_list>
namespace Jobs
{
class JobDataDesc
{
public:
    static const SizeT MaxNumBuffers = 8;

    /// default constructor
    JobDataDesc();
	/// construct from nullptr
	JobDataDesc(nullptr_t);
	/// constructor with array of pointers, but may not exceed MaxNumBuffers
	JobDataDesc(const std::initializer_list<std::tuple<void*, SizeT, SizeT>> data);
    /// constructor with 1 data buffer
    JobDataDesc(void* ptr, SizeT bufSize, SizeT sliceSize);
    /// constructor with 2 data buffers
    JobDataDesc(void* ptr0, SizeT bufSize0, SizeT sliceSize0, void* ptr1, SizeT bufSize1, SizeT sliceSize1);
    /// constructor with 3 data buffers
    JobDataDesc(void* ptr0, SizeT bufSize0, SizeT sliceSize0, void* ptr1, SizeT bufSize1, SizeT sliceSize1, void* ptr2, SizeT bufSize2, SizeT sliceSize2);
    /// constructor with 4 data buffers
    JobDataDesc(void* ptr0, SizeT bufSize0, SizeT sliceSize0, void* ptr1, SizeT bufSize1, SizeT sliceSize1, void* ptr2, SizeT bufSize2, SizeT sliceSize2, void* ptr3, SizeT bufSize3, SizeT sliceSize3);

    /// update a parameter set
    void Update(IndexT index, void* ptr, SizeT bufSize, SizeT sliceSize);

    /// get number of buffers
    SizeT GetNumBuffers() const;
    /// get buffer pointer
    void* GetPointer(IndexT i) const;
    /// get buffer size
    SizeT GetBufferSize(IndexT i) const;
    /// get slice size
    SizeT GetSliceSize(IndexT i) const;

private:
    SizeT numBuffers;
    void* ptr[MaxNumBuffers];
    SizeT bufferSize[MaxNumBuffers];
    SizeT sliceSize[MaxNumBuffers];
};

//------------------------------------------------------------------------------
/**
*/
inline
JobDataDesc::JobDataDesc() :
    numBuffers(0)
{
    IndexT i;
    for (i = 0; i < MaxNumBuffers; i++)
    {
        this->ptr[i] = nullptr;
        this->bufferSize[i] = 0;
        this->sliceSize[i] = 0;
    }
}
//------------------------------------------------------------------------------
/**
*/
inline 
JobDataDesc::JobDataDesc(nullptr_t) :
	numBuffers(0)
{
	IndexT i;
	for (i = 0; i < MaxNumBuffers; i++)
	{
		this->ptr[i] = nullptr;
		this->bufferSize[i] = 0;
		this->sliceSize[i] = 0;
	}
}

//------------------------------------------------------------------------------
/**
*/
inline
JobDataDesc::JobDataDesc(const std::initializer_list<std::tuple<void*, SizeT, SizeT>> data)
{
	n_assert(data.size() <= MaxNumBuffers);
	this->numBuffers = (SizeT)data.size();

	size_t i;
	for (i = 0; i < data.size(); i++)
	{
		const std::tuple<void*, SizeT, SizeT>& d = data.begin()[i];
		this->ptr[i] = std::get<0>(d);
		this->bufferSize[i] = std::get<1>(d);
		this->sliceSize[i] = std::get<2>(d);
	}
}

//------------------------------------------------------------------------------
/**
*/
inline
JobDataDesc::JobDataDesc(void* ptr_, SizeT bufSize_, SizeT sliceSize_) :
    numBuffers(1)
{
    this->ptr[0] = ptr_;
    this->bufferSize[0] = bufSize_;
    n_assert(sliceSize_ <= JobMaxSliceSize);
    this->sliceSize[0] = sliceSize_;
    
    IndexT i;
    for (i = 1; i < MaxNumBuffers; i++)
    {
        this->ptr[i] = 0;
        this->bufferSize[i] = 0;
        this->sliceSize[i] = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
JobDataDesc::JobDataDesc(void* ptr0_, SizeT bufSize0_, SizeT sliceSize0_, void* ptr1_, SizeT bufSize1_, SizeT sliceSize1_) :
    numBuffers(2)
{
    this->ptr[0] = ptr0_;
    this->bufferSize[0] = bufSize0_;
    n_assert(sliceSize0_ <= JobMaxSliceSize);
    this->sliceSize[0] = sliceSize0_;

    this->ptr[1] = ptr1_;
    this->bufferSize[1] = bufSize1_;
    n_assert(sliceSize1_ <= JobMaxSliceSize);
    this->sliceSize[1] = sliceSize1_;

    IndexT i;
    for (i = 2; i < MaxNumBuffers; i++)
    {
        this->ptr[i] = 0;
        this->bufferSize[i] = 0;
        this->sliceSize[i] = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
JobDataDesc::JobDataDesc(void* ptr0_, SizeT bufSize0_, SizeT sliceSize0_, void* ptr1_, SizeT bufSize1_, SizeT sliceSize1_, void* ptr2_, SizeT bufSize2_, SizeT sliceSize2_) :
    numBuffers(3)
{
    this->ptr[0] = ptr0_;
    this->bufferSize[0] = bufSize0_;
    n_assert(sliceSize0_ <= JobMaxSliceSize);
    this->sliceSize[0] = sliceSize0_;

    this->ptr[1] = ptr1_;
    this->bufferSize[1] = bufSize1_;
    n_assert(sliceSize1_ <= JobMaxSliceSize);
    this->sliceSize[1] = sliceSize1_;

    this->ptr[2] = ptr2_;
    this->bufferSize[2] = bufSize2_;
    n_assert(sliceSize2_ <= JobMaxSliceSize);
    this->sliceSize[2] = sliceSize2_;

    this->ptr[3] = 0;
    this->bufferSize[3] = 0;
    this->sliceSize[3] = 0;
}

//------------------------------------------------------------------------------
/**
*/
inline
JobDataDesc::JobDataDesc(void* ptr0_, SizeT bufSize0_, SizeT sliceSize0_, void* ptr1_, SizeT bufSize1_, SizeT sliceSize1_, void* ptr2_, SizeT bufSize2_, SizeT sliceSize2_, void* ptr3_, SizeT bufSize3_, SizeT sliceSize3_) :
    numBuffers(4)
{
    this->ptr[0] = ptr0_;
    this->bufferSize[0] = bufSize0_;
    n_assert(sliceSize0_ <= JobMaxSliceSize);
    this->sliceSize[0] = sliceSize0_;

    this->ptr[1] = ptr1_;
    this->bufferSize[1] = bufSize1_;
    n_assert(sliceSize1_ <= JobMaxSliceSize);
    this->sliceSize[1] = sliceSize1_;

    this->ptr[2] = ptr2_;
    this->bufferSize[2] = bufSize2_;
    n_assert(sliceSize2_ <= JobMaxSliceSize);
    this->sliceSize[2] = sliceSize2_;

    this->ptr[3] = ptr3_;
    this->bufferSize[3] = bufSize3_;
    n_assert(sliceSize3_ <= JobMaxSliceSize);
    this->sliceSize[3] = sliceSize3_;
}

//------------------------------------------------------------------------------
/**
*/
inline void
JobDataDesc::Update(IndexT index, void* ptr_, SizeT bufSize_, SizeT sliceSize_)
{
    n_assert(index < this->numBuffers);
    this->ptr[index] = ptr_;
    this->bufferSize[index] = bufSize_;
    n_assert(sliceSize_ <= JobMaxSliceSize);
    this->sliceSize[index] = sliceSize_;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
JobDataDesc::GetNumBuffers() const
{
    return this->numBuffers;
}

//------------------------------------------------------------------------------
/**
*/
inline void*
JobDataDesc::GetPointer(IndexT i) const
{
    n_assert(i < MaxNumBuffers);
    return this->ptr[i];
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
JobDataDesc::GetBufferSize(IndexT i) const
{
    n_assert(i < MaxNumBuffers);
    return this->bufferSize[i];
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
JobDataDesc::GetSliceSize(IndexT i) const
{
    n_assert(i < MaxNumBuffers);
    n_assert(this->sliceSize[i] <= JobMaxSliceSize);
    return this->sliceSize[i];
}

} // namespace Jobs
//------------------------------------------------------------------------------
    