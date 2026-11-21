#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::UnpackPrefabCommand

    Unlinks one prefab instantiation. The entities keep every value they hold and become ordinary
    authored entities, and the command keeps the links and the instance so an undo puts the
    instantiation back exactly.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/ptr.h"
#include "core/refcounted.h"
#include "neweditor/model/editcommand.h"
#include "neweditor/model/scene.h"
#include "game/entity.h"
#include "util/array.h"
#include "util/guid.h"

namespace Editor
{

class UnpackPrefabCommand : public EditCommand
{
    __DeclareClass(UnpackPrefabCommand)
public:
    const char* Name() const override;
    bool Execute() override;
    bool Unexecute() override;

    Ptr<Scene> scene;
    /// any entity of the instantiation. The whole instantiation goes
    Game::Entity entity;

private:
    /// one entity's link, as it was while the instance is unlinked
    struct EntityLink
    {
        Game::Entity entity;
        Util::Guid prefabEntity;
        Util::Array<PrefabOverride> overrides;
    };

    bool initialized = false;
    IndexT instanceIndex = InvalidIndex;
    PrefabInstance instanceRecord;
    Util::Array<EntityLink> links;
};

} // namespace Editor
