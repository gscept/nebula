#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::InstantiatePrefabCommand

    Instantiates a prefab file into a scene. The first run reads the file and creates the entities
    and the collections it carries; what it made is then kept as records and component values, so a
    redo does not read the file again but puts back the entities as they were, with the same ids.
    While the entities have no instance, the command holds their ids, the way the create command
    does.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/ptr.h"
#include "core/refcounted.h"
#include "neweditor/model/editcommand.h"
#include "neweditor/model/scene.h"
#include "game/componentid.h"
#include "game/entity.h"
#include "util/array.h"
#include "util/blob.h"
#include "util/guid.h"
#include "util/string.h"

namespace Editor
{

class InstantiatePrefabCommand : public EditCommand
{
    __DeclareClass(InstantiatePrefabCommand)
public:
    virtual ~InstantiatePrefabCommand();

    const char* Name() const override;
    bool Execute() override;
    bool Unexecute() override;

    Ptr<Scene> scene;
    /// the prefab file to instantiate
    Util::String path;
    /// the collection the entities go into. Invalid means the active one
    Util::Guid collection;
    /// the name given to the instantiation's root entity. Empty keeps the names the file has
    Util::String rootName;
    /// the entities the run made, in file order. The panels read it after a drop to know what was
    /// made and
    /// which of them are the roots
    Util::Array<Game::Entity> created;

private:
    /// one entity of the instantiation, as it was while the instance is out
    struct EntitySnapshot
    {
        Editable record;
        Util::Array<Game::ComponentId> components;
        Util::Array<Util::Blob> values;
    };

    /// read from the file and create. False when the file cannot be read or is broken
    bool InstantiateFromDocument();
    /// put the entities back from the snapshots, which is what a redo does
    void InstantiateFromSnapshots();
    /// the collections the file brought with it, kept as records so the redo puts the same
    /// ones back and the entity records still name them
    void CaptureCollections(const PrefabInstance& instance);

    Util::Array<EntitySnapshot> snapshots;
    Util::Array<Collection> collections;
};

} // namespace Editor
