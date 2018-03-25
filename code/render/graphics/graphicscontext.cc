//------------------------------------------------------------------------------
// graphicscontext.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "graphicscontext.h"

namespace Graphics
{

__ImplementAbstractClass(Graphics::GraphicsContext, 'GRCO', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
GraphicsContext::GraphicsContext()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
GraphicsContext::~GraphicsContext()
{
	// empty
}


//------------------------------------------------------------------------------
/**
*/
void
GraphicsContext::RegisterEntity(const GraphicsEntityId id)
{
	n_assert(!this->entitySliceMap.Contains(id.id));
	ContextEntityId allocId = this->Alloc();
	this->entitySliceMap.Add(id, allocId);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsContext::DeregisterEntity(const GraphicsEntityId id)
{
	IndexT i = this->entitySliceMap.FindIndex(id.id);
	n_assert(i != InvalidIndex);
	this->Dealloc(this->entitySliceMap.ValueAtIndex(i));
	this->entitySliceMap.Erase(i);
}

//------------------------------------------------------------------------------
/**
*/
bool
GraphicsContext::IsEntityRegistered(const GraphicsEntityId id)
{
	return this->entitySliceMap.Contains(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsContext::OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime)
{
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsContext::OnVisibilityReady(const IndexT frameIndex, const Timing::Time frameTime)
{
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsContext::OnBeforeView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime)
{
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsContext::OnAfterView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime)
{
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsContext::OnAfterFrame(const IndexT frameIndex, const Timing::Time frameTime)
{
	// implement in subclass
}

} // namespace Graphics