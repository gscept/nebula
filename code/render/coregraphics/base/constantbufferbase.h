#pragma once
//------------------------------------------------------------------------------
/**
	@class Base::ShaderConstantBufferBase
	
	A buffer which represents a set of shader constant variables 
    (uniforms in OpenGL) which are contained within a buffer object.

	Good practice is to have one constant buffer, then create instances using 
	AllocateInstance, which returns an offset into the buffer where the new instance is.

	Constant buffers can be set to be synchronized or non-synchronized.

	Synchronized: will cause a GPU synchronizing command whenever the EndUpdateSync gets run.
	Non-synchronized: will cause all previous commands to be flushed before continuing, when running EndUpdateSync.

	Non-synchronized buffers are awesome if you only need to update your buffer once per frame.
	However, if you use a non-synchronized buffer which you update more than once per frame 
	and you don't have enough backing buffers to cycle last for each update, then this buffer
	will cause a complete frame flush.

	Synchronized buffers are good for when you have no idea when a buffer gets updated. They let 
	the driver take care of when the buffer is needed and will as such cause a direct flush.

	This type of behavior might only be relevant in non-direct APIs such as DX11 and OpenGL, since
	we can't actually tell whenever the buffer gets updated with a persistently mapped buffer.
	
	(C) 2015-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/stringatom.h"
#include "coregraphics/shadervariable.h"
#include "coregraphics/stretchybuffer.h"

namespace CoreGraphics
{
class Shader;
}

namespace Base
{
class ConstantBufferBase : public CoreGraphics::StretchyBuffer
{
	__DeclareAbstractClass(ConstantBufferBase);
public:
	/// constructor
	ConstantBufferBase();
	/// destructor
	virtual ~ConstantBufferBase();

    /// setup buffer
    void Setup(const SizeT numBackingBuffers = DefaultNumBackingBuffers);
    /// bind variables in a block with a name in a shader to this buffer (only do this on system managed blocks)
	void SetupFromBlockInShader(const Ptr<CoreGraphics::ShaderState>& shader, const Util::String& blockName, const SizeT numBackingBuffers = DefaultNumBackingBuffers);
    /// discard buffer
    void Discard();

	/// set active offset, this will be the base offset when updating variables in this block
	void SetBaseOffset(SizeT offset);

    /// set if this buffer should be updated synchronously, the default behavior is not
    void SetSync(bool b);

    /// get variable inside constant buffer by name
    const Ptr<CoreGraphics::ShaderVariable>& GetVariableByName(const Util::StringAtom& name) const;

    /// set the size, must be done prior to setting it up
    void SetSize(uint size);

    /// returns buffer handle
    void* GetHandle() const;

    /// begin updating the buffer using a synchronous method
    void BeginUpdateSync();
    /// update buffer, if not within begin/end synced update, it will be an asynchronous update
    template <class T> void Update(const T& data, uint offset, uint size);
    /// update buffer using an array, if not within begin/end synced update, it will be an asynchronous update
    template <class T> void UpdateArray(const T data, uint offset, uint size, uint count);
    /// end updating the buffer using a synchronous method
    void EndUpdateSync();

    /// cycle to next buffer
    void CycleBuffers();

    static const int DefaultNumBackingBuffers = 3;

protected:

    /// update buffer asynchronously, depending on implementation, this might overwrite data before used
    virtual void UpdateAsync(void* data, uint offset, uint size);
    /// update segment of buffer as array, depending on implementation, this might overwrite data before used
    virtual void UpdateArrayAsync(void* data, uint offset, uint size, uint count);

    /// update buffer synchronously
    virtual void UpdateSync(void* data, uint offset, uint size);
    /// update buffer synchronously using an array of data
    virtual void UpdateArraySync(void* data, uint offset, uint size, uint count);

    Util::Array<Ptr<CoreGraphics::ShaderVariable>> variables;
    Util::Dictionary<Util::StringAtom, Ptr<CoreGraphics::ShaderVariable>> variablesByName;
    bool isSetup;

	// size is total memory size of entire buffer, stride is size of single instance
    IndexT bufferIndex;
	SizeT numBuffers;
	IndexT baseOffset;

    bool sync;
    bool inUpdateSync;
    bool isDirty;
    void* buffer;
};

//------------------------------------------------------------------------------
/**
*/
inline void
ConstantBufferBase::SetSync(bool b)
{
	n_assert(!this->isSetup);
    this->sync = b;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Base::ConstantBufferBase::UpdateAsync(void* data, uint offset, uint size)
{
     // override this in subclass where we might, or might not have the ability to update a buffer asynchronously
}

//------------------------------------------------------------------------------
/**
*/
inline void
Base::ConstantBufferBase::UpdateArrayAsync(void* data, uint offset, uint size, uint count)
{
    // override this in subclass where we might, or might not have the ability to update a buffer asynchronously
}

//------------------------------------------------------------------------------
/**
*/
inline void
Base::ConstantBufferBase::UpdateSync(void* data, uint offset, uint size)
{
    n_assert(this->inUpdateSync);   
	byte* buf = (byte*)this->buffer + offset + this->baseOffset;
    memcpy(buf, data, size);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Base::ConstantBufferBase::UpdateArraySync(void* data, uint offset, uint size, uint count)
{
    n_assert(this->inUpdateSync);
    byte* buf = (byte*)this->buffer + offset + this->baseOffset;
    memcpy(buf, data, size * count);
}

//------------------------------------------------------------------------------
/**
*/
template <class T>
inline void
Base::ConstantBufferBase::Update(const T& data, uint offset, uint size)
{
    n_assert(0 != this->buffer);
    switch (this->inUpdateSync)
    {
    case true:  this->UpdateSync((void*)&data, offset, size); break;
    case false: this->UpdateAsync((void*)&data, offset, size); break;
    }
    this->isDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
template <class T>
inline void
Base::ConstantBufferBase::UpdateArray(const T data, uint offset, uint size, uint count)
{
    n_assert(0 != this->buffer);
    switch (this->inUpdateSync)
    {
	case true:  this->UpdateSync((void*)data, offset, size * count); break;
	case false: this->UpdateAsync((void*)data, offset, size * count); break;
    }
    this->isDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ConstantBufferBase::SetSize(uint size)
{
    this->size = size;
}

//------------------------------------------------------------------------------
/**
*/
inline void*
ConstantBufferBase::GetHandle() const
{
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::ShaderVariable>&
ConstantBufferBase::GetVariableByName(const Util::StringAtom& name) const
{
    return this->variablesByName[name];
}

//------------------------------------------------------------------------------
/**
*/
inline void
ConstantBufferBase::SetBaseOffset(SizeT offset)
{
	this->baseOffset = offset;
}

} // namespace Base