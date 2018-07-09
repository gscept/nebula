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

	(C) 2017 Individual contributors, see AUTHORS file
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
ConstantBufferSliceId ConstantBufferAllocateInstance(const ConstantBufferId id);
/// free an instance
void ConstantBufferFreeInstance(const ConstantBufferId id, ConstantBufferSliceId slice);
/// reset instances in constant buffer
void ConstantBufferResetInstances(const ConstantBufferId id);

/// get constant buffer slot from reflection
IndexT ConstantBufferGetSlot(const ConstantBufferId id);

/// update constant buffer
void ConstantBufferUpdate(const ConstantBufferId id, const void* data, const uint offset, const uint size);
/// update constant buffer using array of objects
void ConstantBufferArrayUpdate(const ConstantBufferId id, const void* data, const uint offset, const uint size, const uint count);
/// update constant buffer
template<class TYPE> void ConstantBufferUpdate(const ConstantBufferId id, const TYPE* data, const uint offset);
/// update constant buffer using array of objects
template<class TYPE> void ConstantBufferArrayUpdate(const ConstantBufferId id, const TYPE* data, const uint offset, const uint count);
/// update constant buffer
void ConstantBufferUpdate(const ConstantBufferId id, const ShaderConstantId cid, const uint size, const void* data);
/// update constant buffer using array of objects
void ConstantBufferArrayUpdate(const ConstantBufferId id, const ShaderConstantId cid, const void* data, const uint size, const uint count);
/// update constant buffer
template<class TYPE> void ConstantBufferUpdate(const ConstantBufferId id, const ShaderConstantId cid, const TYPE* data);
/// update constant buffer using array of objects
template<class TYPE> void ConstantBufferArrayUpdate(const ConstantBufferId id, const ShaderConstantId cid, const TYPE* data, const uint count);
/// set base offset for constant buffer
void ConstantBufferSetBaseOffset(const ConstantBufferId id, const uint offset);

/// create variable directly on this constant buffer
const ShaderConstantId ConstantBufferCreateShaderVariable(const ConstantBufferId id, const ConstantBufferSliceId slice, const Util::StringAtom& name);
/// destroy variable on this constant buffer
void ConstantBufferDestroyShaderVariable(const ConstantBufferId id, const ShaderConstantId var);

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
ConstantBufferUpdate(const ConstantBufferId id, const TYPE* data, const uint offset)
{
	ConstantBufferUpdate(id, data, offset, sizeof(TYPE));
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
ConstantBufferArrayUpdate(const ConstantBufferId id, const TYPE* data, const uint offset, const uint count)
{
	ConstantBufferArrayUpdate(id, data, offset, sizeof(TYPE), count);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
ConstantBufferUpdate(const ConstantBufferId id, const ShaderConstantId cid, const TYPE* data)
{
	ConstantBufferUpdate(id, cid, data, sizeof(TYPE));
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
ConstantBufferArrayUpdate(const ConstantBufferId id, const ShaderConstantId cid, const TYPE* data, const uint count)
{
	ConstantBufferArrayUpdate(id, cid, data, sizeof(TYPE), count);
}

} // CoreGraphics
