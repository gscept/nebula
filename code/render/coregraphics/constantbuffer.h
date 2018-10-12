#pragma once
//------------------------------------------------------------------------------
/**
	Constant buffer interface. A constant buffer is a resource type which provides
	the shader with its state. In essence, constants can be updated with different
	frequency, and is more easily shared, if stored in a buffer like format.

	The constant buffers work by extending their size when a new instance is 
	allocated, and the assigned slice is returned. This allows the same
	buffer to be bound to the graphics device but still be able to accomodate
	new objects with new unique constants.

	(C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idpool.h"
#include "util/stringatom.h"
#include "coregraphics/shader.h"

namespace CoreGraphics
{

ID_24_8_TYPE(ConstantBufferId);
ID_32_TYPE(ConstantBufferSliceId);

struct ConstantBufferCreateInfo
{
	bool setupFromReflection;			// use if created outside a shader state, and is supposed to be attached to a shader within said state

	ShaderId shader;					// the shader state to bind to
	Util::StringAtom name;				// name in shader state for the block
	SizeT size;							// if setupFromReflection is true, this is the number of backing buffers, otherwise, it is the byte size
	SizeT numBuffers;					// declare the amount of buffer rings (double, triple, quadruple buffering...)
};

struct ConstantBufferInfo
{
	SizeT offset;
};

/// create new constant buffer
const ConstantBufferId CreateConstantBuffer(const ConstantBufferCreateInfo& info);
/// destroy constant buffer
void DestroyConstantBuffer(const ConstantBufferId id);
/// allocate an instance of this buffer
bool ConstantBufferAllocateInstance(const ConstantBufferId id, uint& offset, uint& slice);
/// free an instance
void ConstantBufferFreeInstance(const ConstantBufferId id, uint slice);
/// reset instances in constant buffer
void ConstantBufferResetInstances(const ConstantBufferId id);

/// get constant buffer slot from reflection
IndexT ConstantBufferGetSlot(const ConstantBufferId id);

/// update constant buffer data
void ConstantBufferUpdate(const ConstantBufferId id, const void* data, const uint size, ConstantBinding bind);
/// update constant buffer data as array
void ConstantBufferUpdateArray(const ConstantBufferId id, const void* data, const uint size, const uint count, ConstantBinding bind);
/// update constant buffer data
template<class TYPE> void ConstantBufferUpdate(const ConstantBufferId id, const TYPE data, ConstantBinding bind);
/// update constant buffer data as array
template<class TYPE> void ConstantBufferUpdateArray(const ConstantBufferId id, const TYPE* data, const uint count, ConstantBinding bind);
/// update constant buffer data instanced
void ConstantBufferUpdateInstance(const ConstantBufferId id, const void* data, const uint size, const uint instance, ConstantBinding bind);
/// update constant buffer data as array instanced
void ConstantBufferUpdateArrayInstance(const ConstantBufferId id, const void* data, const uint size, const uint count, const uint instance, ConstantBinding bind);
/// update constant buffer data instanced
template<class TYPE> void ConstantBufferUpdateInstance(const ConstantBufferId id, const TYPE data, const uint instance, ConstantBinding bind);
/// update constant buffer data as array instanced
template<class TYPE> void ConstantBufferUpdateArrayInstance(const ConstantBufferId id, const TYPE* data, const uint count, const uint instance, ConstantBinding bind);

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void ConstantBufferUpdate(const ConstantBufferId id, const TYPE data, ConstantBinding bind)
{
	ConstantBufferUpdate(id, &data, sizeof(TYPE), bind);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void ConstantBufferUpdateArray(const ConstantBufferId id, const TYPE* data, const uint count, ConstantBinding bind)
{
	ConstantBufferUpdateArray(id, data, sizeof(TYPE), count, bind);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void ConstantBufferUpdateInstance(const ConstantBufferId id, const TYPE data, const uint instance, ConstantBinding bind)
{
	ConstantBufferUpdateInstance(id, &data, sizeof(TYPE), instance, bind);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void ConstantBufferUpdateArrayInstance(const ConstantBufferId id, const TYPE* data, const uint count, const uint instance, ConstantBinding bind)
{
	ConstantBufferUpdateArrayInstance(id, data, sizeof(TYPE), count, instance, bind);
}

} // CoreGraphics
