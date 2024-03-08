//------------------------------------------------------------------------------
//  @file pickingcontext.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "pickingcontext.h"
namespace Editor
{

PickingContext::PickingIdAllocator PickingContext::allocator;
__ImplementContext(PickingContext, PickingContext::allocator)

//------------------------------------------------------------------------------
/**
*/
PickingContext::PickingContext()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
PickingContext::~PickingContext()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
PickingContext::SetupEntity(Graphics::GraphicsEntityId id, Game::Entity editorEntity)
{
	n_assert(id != Graphics::GraphicsEntityId::Invalid());

	Graphics::ContextEntityId const cid = GetContextId(id);

	allocator.Get<0>(cid.id) = editorEntity;

	
}

} // namespace Editor
