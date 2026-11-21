//------------------------------------------------------------------------------
//  @file pasteentitiescommand.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/commands/pasteentitiescommand.h"
#include "game/component.h"
#include "game/world.h"
#include "memdb/attributeregistry.h"

namespace Editor
{

__ImplementClass(Editor::PasteEntitiesCommand, 'PPEC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
    While the entities have no instance, their ids are the command's, and releasing them here is
    what makes the paste actually gone.
*/
PasteEntitiesCommand::~PasteEntitiesCommand()
{
    if (!this->executed)
    {
        for (IndexT i = 0; i < this->snapshots.Size(); i++)
            this->scene->DeallocateEntityId(this->snapshots[i].record.entity);
    }
}

//------------------------------------------------------------------------------
/**
*/
const char*
PasteEntitiesCommand::Name() const
{
    return this->action.AsCharPtr();
}

//------------------------------------------------------------------------------
/**
*/
bool
PasteEntitiesCommand::Execute()
{
    if (!this->initialized)
        return this->PasteFromDocument();

    this->PasteFromSnapshots();
    return true;
}

//------------------------------------------------------------------------------
/**
    The instances go and the ids stay, the way the create command puts an entity out.
*/
bool
PasteEntitiesCommand::Unexecute()
{
    for (IndexT i = 0; i < this->snapshots.Size(); i++)
        this->scene->DeallocateInstance(this->snapshots[i].record.entity);

    return true;
}

//------------------------------------------------------------------------------
/**
    What the run made is kept as records and values, which is what makes a redo put back the same
    entities with the same ids rather than read the document again.
*/
bool
PasteEntitiesCommand::PasteFromDocument()
{
    this->created.Clear();
    if (!this->scene->PasteDocument(this->document, this->collection, this->parent, this->created))
        return false;

    // The text is captured from the entities the paste just made, and a text that names entities
    // names them
    // by the guids the scene has for them, which is what the redo resolves the same text back
    // through
    Util::HashTable<Util::Guid, Game::Entity> map;
    this->scene->BuildSceneMap(map);

    for (IndexT i = 0; i < this->created.Size(); i++)
    {
        EntitySnapshot snapshot;
        snapshot.record = this->scene->GetEditable(this->created[i]);

        const Util::Array<Game::ComponentId>& components = this->scene->GetComponents(this->created[i]);
        for (IndexT c = 0; c < components.Size(); c++)
        {
            const Game::ComponentId component = components[c];
            if (component == Game::GetComponentId<Game::Entity>())
                continue;

            snapshot.components.Append(component);

            const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(component);
            if (typeSize > 0)
                snapshot.values.Append(Util::Blob(this->scene->GetComponentData(this->created[i], component), typeSize));
            else
                snapshot.values.Append(Util::Blob());
        }

        // The text the extensions produced for the entity the paste just made. A redo has to put
        // that
        // back too, and it is the only thing it has to put it back from
        this->scene->CaptureExtensions(this->created[i], snapshot.extensions, map);

        this->snapshots.Append(snapshot);
    }

    this->initialized = true;
    return true;
}

//------------------------------------------------------------------------------
/**
    The same entities, with the same ids and the same values.
*/
void
PasteEntitiesCommand::PasteFromSnapshots()
{
    // The text was captured from the entities this command made, so it names their guids, and the
    // scene's
    // own map is what answers them back
    Util::HashTable<Util::Guid, Game::Entity> map;
    this->scene->BuildSceneMap(map);

    for (IndexT i = 0; i < this->snapshots.Size(); i++)
    {
        const EntitySnapshot& snapshot = this->snapshots[i];
        this->scene->AllocateInstance(snapshot.record);

        for (IndexT c = 0; c < snapshot.components.Size(); c++)
        {
            const Util::Blob& value = snapshot.values[c];
            this->scene->AddComponent(snapshot.record.entity, snapshot.components[c],
                value.IsValid() ? value.GetPtr() : nullptr);
        }

        // The same order as the paste: the components first, then the text that has to create
        // something
        // on an entity that has just been made
        this->scene->ApplyExtensions(snapshot.record.name, snapshot.extensions, snapshot.record.entity, map);
    }
}

} // namespace Editor
