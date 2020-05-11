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

	(C)2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idpool.h"
#include "util/stringatom.h"
#include "util/variant.h"
#include "coregraphics/config.h"

namespace CoreGraphics
{

ID_24_8_TYPE(ConstantBufferId);
ID_32_32_NAMED_TYPE(ConstantBufferAllocId, offset, size);

typedef uint ConstantBinding; // defined again!
static uint InvalidConstantBinding = UINT_MAX;

struct ConstantBufferCreateInfo
{
	Util::StringAtom name;				// name of the constant buffer block
	IndexT binding;						// binding slot of the constant buffer
	SizeT size;							// allocation size of the buffer
	BufferUpdateMode mode;
};

/// create new constant buffer
const ConstantBufferId CreateConstantBuffer(const ConstantBufferCreateInfo& info);
/// destroy constant buffer
void DestroyConstantBuffer(const ConstantBufferId id);

/// get constant buffer slot from reflection
IndexT ConstantBufferGetSlot(const ConstantBufferId id);

/// update constant buffer data
void ConstantBufferUpdate(const ConstantBufferId id, const void* data, const uint size, ConstantBinding bind);
/// update constant buffer data as array
void ConstantBufferUpdateArray(const ConstantBufferId id, const void* data, const uint size, const uint count, ConstantBinding bind);
/// update constant buffer data
template<class TYPE> void ConstantBufferUpdate(const ConstantBufferId id, const TYPE& data, ConstantBinding bind);
/// update constant buffer data as array
template<class TYPE> void ConstantBufferUpdateArray(const ConstantBufferId id, const TYPE* data, const uint count, ConstantBinding bind);

/// update constant buffer using range of memory
void ConstantBufferUpdate(const ConstantBufferId id, const ConstantBufferAllocId alloc, const void* data, const uint size, ConstantBinding bind);

/// flush changes made to constant buffer
void ConstantBufferFlush(const ConstantBufferId id);

//------------------------------------------------------------------------------
/**
*/
template<>
inline void ConstantBufferUpdate(const ConstantBufferId id, const Util::Variant& data, ConstantBinding bind)
{
	ConstantBufferUpdate(id, data.AsVoidPtr(), data.Size(), bind);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void ConstantBufferUpdate(const ConstantBufferId id, const TYPE& data, ConstantBinding bind)
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


} // CoreGraphics
