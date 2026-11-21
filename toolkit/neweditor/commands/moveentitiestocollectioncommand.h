#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::MoveEntitiesToCollectionCommand

    Moves authored entities into a collection. The collections the entities were in are taken on the
    first run, in the same order as the entities, so a redo puts each one back where it was.

    This is the command a drag in the outline is: a drag that moves ten entities is one entry.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/ptr.h"
#include "neweditor/model/editcommand.h"
#include "neweditor/model/scene.h"
#include "game/entity.h"
#include "util/array.h"
#include "util/guid.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
*/
class MoveEntitiesToCollectionCommand : public EditCommand
{
    __DeclareClass(MoveEntitiesToCollectionCommand)
public:
    const char* Name() const override;
    bool Execute() override;
    bool Unexecute() override;

    Ptr<Scene> scene;
    Util::Array<Game::Entity> entities;
    /// an invalid collection is the scene root
    Util::Guid newCollection;

private:
    /// the collection each entity was in, in the same order as the entities
    Util::Array<Util::Guid> oldCollections;
    bool initialized;
};

} // namespace Editor
