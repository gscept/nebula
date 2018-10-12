#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::FeedbackBufferBase
  
    A resource which contains data done after a transform feedback/stream output operation is done
    
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "coregraphics/vertexlayout.h"
#include "coregraphics/base/resourcebase.h"
#include "coregraphics/primitivegroup.h"

//------------------------------------------------------------------------------
namespace Base
{
class FeedbackBufferBase : public Core::RefCounted
{
	__DeclareClass(FeedbackBufferBase);
public:
    /// constructor
	FeedbackBufferBase();
    /// destructor
	virtual ~FeedbackBufferBase();

    /// setup buffer
	void Setup();
	/// discard buffer
	void Discard();
    /// map the vertices for CPU access
    void* Map(GpuResourceBase::MapType mapType);
    /// unmap the resource
    void Unmap();

	/// swap buffers
	void Swap();
    
	/// set number of primitives
    void SetNumPrimitives(SizeT numPrimitives);
    /// get number of vertices in the buffer
    SizeT GetNumPrimitives() const;
	/// set type of primitive
	void SetPrimitiveType(CoreGraphics::PrimitiveTopology::Code prim);
	/// set vertex components used for buffer
	void SetVertexComponents(const Util::Array<CoreGraphics::VertexComponent>& comps);

	/// get primitive group
	const CoreGraphics::PrimitiveGroup& GetPrimitiveGroup() const;
	/// get vertex layout
	const Ptr<CoreGraphics::VertexLayout>& GetLayout() const;

protected:
	SizeT size;
    SizeT numPrimitives;
	SizeT numElements;
	CoreGraphics::PrimitiveGroup primGroup;
	CoreGraphics::PrimitiveTopology::Code prim;
	Util::Array<CoreGraphics::VertexComponent> components;
	Ptr<CoreGraphics::VertexLayout> layout;
};

//------------------------------------------------------------------------------
/**
*/
inline void
FeedbackBufferBase::SetNumPrimitives(SizeT num)
{
    n_assert(num > 0);
	this->numPrimitives = num;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
FeedbackBufferBase::GetNumPrimitives() const
{
	return this->numPrimitives;
}

//------------------------------------------------------------------------------
/**
*/
inline void
FeedbackBufferBase::SetPrimitiveType(CoreGraphics::PrimitiveTopology::Code prim)
{
	n_assert(prim != CoreGraphics::PrimitiveTopology::InvalidPrimitiveTopology);
	this->prim = prim;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::PrimitiveGroup&
FeedbackBufferBase::GetPrimitiveGroup() const
{
	return this->primGroup;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::VertexLayout>&
FeedbackBufferBase::GetLayout() const
{
	return this->layout;
}

//------------------------------------------------------------------------------
/**
*/
inline void
FeedbackBufferBase::SetVertexComponents(const Util::Array<CoreGraphics::VertexComponent>& comps)
{
	this->components = comps;
}

} // namespace Base
//------------------------------------------------------------------------------

