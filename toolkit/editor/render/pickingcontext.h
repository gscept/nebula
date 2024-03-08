#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::PickingContext

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "game/entity.h"

namespace Editor
{

class PickingContext : public Graphics::GraphicsContext
{
    __DeclareContext()

public:
	PickingContext();
	~PickingContext();

    void SetupEntity(Graphics::GraphicsEntityId id, Game::Entity editorEntity);

private:
    typedef Ids::IdAllocator<
        Game::Entity // Maps directly to editor game entity
    > PickingIdAllocator;
    static PickingIdAllocator allocator;
};

} // namespace Editor
