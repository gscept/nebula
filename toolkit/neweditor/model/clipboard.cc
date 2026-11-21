//------------------------------------------------------------------------------
//  @file clipboard.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/model/clipboard.h"
#include "neweditor/commands/pasteentitiescommand.h"
#include "neweditor/model/editcommandmanager.h"
#include "neweditor/api/shell.h"
#include "neweditor/base/editorlog.h"
#include "neweditor/model/scene.h"
#include "game/component.h"
#include "game/entity.h"
#include "game/levelextension.h"
#include "imgui.h"
#include "memdb/attributeregistry.h"
#include "util/stringatom.h"

namespace Editor
{

__ImplementClass(Editor::Clipboard, 'CLPB', Core::RefCounted);

static const Util::StringAtom EntityType("Game::Entity");

//------------------------------------------------------------------------------
/**
*/
Clipboard::Clipboard() :
    core(nullptr)
{
}

//------------------------------------------------------------------------------
/**
*/
Clipboard::~Clipboard()
{
    this->core = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
Clipboard::Setup(Shell* core)
{
    this->core = core;
}

//------------------------------------------------------------------------------
/**
*/
void
Clipboard::Discard()
{
    this->core = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<SceneDocument>
Clipboard::BuildDocument(Scene* scene, const Util::Array<Game::Entity>& entities,
    Util::Array<Util::Guid>& guidByEntityIndex)
{
    if (entities.IsEmpty())
        return nullptr;

    const Util::Array<Editable>& editables = scene->GetEditables();
    const IndexT num = editables.Size();

    // The child lists, from the parent field. A node is its index in the editables, which is the
    // entity index, because the editables are indexed by entity index
    Util::Array<bool> included;
    Util::Array<IndexT> childHead;
    Util::Array<IndexT> childLast;
    Util::Array<IndexT> childNext;
    included.Resize(num, false);
    childHead.Resize(num, InvalidIndex);
    childLast.Resize(num, InvalidIndex);
    childNext.Resize(num, InvalidIndex);

    for (IndexT i = 0; i < num; i++)
    {
        if (!editables[i].guid.IsValid() || !scene->HasInstance(editables[i].entity))
            continue;

        Game::Entity parent = scene->GetParent(editables[i].entity);
        if (parent == Game::Entity::Invalid() || parent.index >= num)
            continue;

        if (childHead[parent.index] == InvalidIndex)
            childHead[parent.index] = i;
        else
            childNext[childLast[parent.index]] = i;
        childLast[parent.index] = i;
    }

    // The selection first, then the subtree under each of it. The walk is a queue over the order
    // array itself, which keeps a parent before its children
    Util::Array<IndexT> order;
    for (IndexT i = 0; i < entities.Size(); i++)
    {
        const Game::Entity entity = entities[i];
        if (entity.index >= num || included[entity.index])
            continue;

        if (!editables[entity.index].guid.IsValid() || !scene->HasInstance(entity))
            continue;

        included[entity.index] = true;
        order.Append(entity.index);
    }

    for (IndexT i = 0; i < order.Size(); i++)
    {
        for (IndexT child = childHead[order[i]]; child != InvalidIndex; child = childNext[child])
        {
            if (included[child])
                continue;

            included[child] = true;
            order.Append(child);
        }
    }

    Ptr<SceneDocument> doc = SceneDocument::Create();

    // Indexed by entity index
    guidByEntityIndex.Clear();
    guidByEntityIndex.Resize(num, Util::Guid());
    for (IndexT i = 0; i < order.Size(); i++)
        guidByEntityIndex[order[i]] = editables[order[i]].guid;

    const Game::ComponentId ownerId = Game::GetComponentId<Game::Entity>();

    // The text the extensions produce names entities by the guids the document carries, and a
    // document can
    // only name its own records. A reference to an entity that is not part of the copy is therefore
    // nothing
    // the text can carry, which is the same rule the component references follow above
    Util::HashTable<Util::Guid, Game::Entity> map;
    for (IndexT i = 0; i < order.Size(); i++)
        map.Add(editables[order[i]].guid, editables[order[i]].entity);

    for (IndexT i = 0; i < order.Size(); i++)
    {
        const IndexT entityIndex = order[i];
        const Editable& record = editables[entityIndex];
        const Game::Entity entity = record.entity;

        DocumentEntity docEntity;
        docEntity.guid = record.guid;
        docEntity.name = record.name;

        // The link. A copy that does not carry it pastes an entity that is its own, which is what
        // makes a
        // duplicate of a prefab entity stop following the prefab. The instance guid is the group
        // the paste
        // makes, and the entity guid is the record in the prefab file the chain begins at
        const IndexT instanceIndex = record.prefabInstance;
        const bool linked = instanceIndex != InvalidIndex && instanceIndex < scene->GetPrefabInstances().Size();
        if (linked)
        {
            const PrefabInstance& instance = scene->GetPrefabInstances()[instanceIndex];
            docEntity.instance = instance.guid;
            docEntity.prefabSource = instance.source;
            docEntity.prefabEntity = record.prefabEntity;

            // An instance entity with no counterpart record is the instance's own addition, and the
            // copy
            // has to say that so the paste knows the entity has no record to name
            docEntity.addedToPrefab = !record.prefabEntity.IsValid();
        }

        // The index the reference entries point at is the position in the document
        const IndexT docIndex = doc->entities.Size();

        const Util::Array<Game::ComponentId>& components = scene->GetComponents(entity);
        for (IndexT c = 0; c < components.Size(); c++)
        {
            const Game::ComponentId component = components[c];
            if (component == ownerId)
                continue;

            DocumentComponent docComponent;
            docComponent.component = component;

            if (linked)
            {
                const PrefabOverride& override = scene->FindPrefabOverride(entity, component);
                docComponent.addedToPrefab = override.componentAdded;
                docComponent.removedFromPrefab = override.componentRemoved;

                // Nothing overridden and nothing removed is nothing to say about the component: the
                // chain
                // already gives the entity exactly what it holds, and a node here would make the
                // paste read it
                // as an override
                if (!override.componentAdded && override.overriddenFields.IsNull() && !override.componentRemoved)
                    continue;

                if (override.componentAdded)
                {
                    // A component the instance added is its own from the start, so every field is
                    // one it
                    // authored. The marker only has the fields it wrote, and the rest would come
                    // back as the
                    // registry's default rather than what the entity holds
                    for (IndexT field = 0; field < MAX_COMPONENT_FIELDS; field++)
                        docComponent.presentFields.SetBit((uint64_t)field);
                }
                else
                {
                    docComponent.presentFields = override.overriddenFields;
                }
            }
            else
            {
                for (IndexT field = 0; field < MAX_COMPONENT_FIELDS; field++)
                    docComponent.presentFields.SetBit((uint64_t)field);
            }

            const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(component);
            void* value = typeSize > 0 ? scene->GetComponentData(entity, component) : nullptr;
            if (value == nullptr)
            {
                docEntity.components.Append(docComponent);
                continue;
            }

            docComponent.value = Util::Blob(value, typeSize);

            // An entity field is recorded as a pending reference rather than left as the entity it
            // holds,
            // the way the parse records them, because the paste resolves the reference against the
            // entities
            // it makes and not against the entities the copy came from
            Game::ComponentInterface* interface =
                static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(component));
            if (interface == nullptr)
            {
                docEntity.components.Append(docComponent);
                continue;
            }

            const IndexT numFields = (IndexT)interface->GetNumFields();
            for (IndexT field = 0; field < numFields; field++)
            {
                const Util::StringAtom type(interface->GetFieldTypenames()[field]);
                if (type != EntityType)
                    continue;

                const Game::Entity target = *(const Game::Entity*)((const byte*)value + interface->GetFieldByteOffsets()[field]);

                DocumentEntityRef ref;
                ref.entity = docIndex;
                ref.component = component;
                ref.field = Util::String(interface->GetFieldNames()[field]);
                ref.target = (target != Game::Entity::Invalid() && target.index < num) ?
                    guidByEntityIndex[target.index] : Util::Guid();
                doc->entityRefs.Append(ref);
            }

            docEntity.components.Append(docComponent);
        }

        // The feature data is captured as the text each registered extension produces for the
        // entity. A
        // copy that did not capture it pastes an entity without its properties, because the paste
        // has
        // nothing else to put them back from
        if (linked)
        {
            // Only the keys the marker says the instance authored, which is what a save writes for
            // a linked
            // entity. The rest is what its chain gives, and carrying the whole value here would
            // make the
            // pasted entity stop following the prefab. The keys are also what the paste's marker is
            // built
            // from, so they have to come over with the text
            scene->SetRefContext(map);

            const Util::Array<Game::LevelExtension>& extensions = Game::LevelExtensionRegistry::GetAllExtensions();
            for (IndexT i = 0; i < extensions.Size(); i++)
            {
                const Game::LevelExtension& extension = extensions[i];
                if (extension.captureKeys == nullptr)
                    continue;

                const PrefabOverride& override = scene->FindExtensionOverride(entity, extension.name);
                if (override.extension != extension.name || override.fields.IsEmpty())
                    continue;

                DocumentExtension entry;
                entry.name = extension.name;
                entry.fields = override.fields;
                scene->CaptureExtensionKeys(entity, extension.name, override.fields, entry.json);
                if (entry.json.IsEmpty())
                    continue;

                docEntity.extensions.Append(entry);
            }
        }
        else
        {
            scene->CaptureExtensions(entity, docEntity.extensions, map);
        }

        doc->entities.Append(docEntity);
    }

    return doc;
}

//------------------------------------------------------------------------------
/**
*/
bool
Clipboard::Copy(const Util::Array<Game::Entity>& entities)
{
    if (this->core == nullptr || entities.IsEmpty())
        return false;

    Util::Array<Util::Guid> guidByEntityIndex;
    Ptr<SceneDocument> doc = this->BuildDocument(this->core->GetScene().get(), entities, guidByEntityIndex);
    if (!doc.isvalid())
        return false;

    const Util::String json = doc->ToJson(guidByEntityIndex);
    if (json.IsEmpty())
        return false;

    ImGui::SetClipboardText(json.AsCharPtr());

    Util::String msg;
    msg.Format("Copied %d entities", (int)doc->entities.Size());
    this->core->Log(LogMessageType::N_MESSAGE, msg);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<SceneDocument>
Clipboard::Read() const
{
    if (this->core == nullptr)
        return nullptr;

    const char* text = ImGui::GetClipboardText();
    if (text == nullptr)
        return nullptr;

    Ptr<SceneDocument> doc = SceneDocument::FromJson(Util::String(text));
    return doc;
}

//------------------------------------------------------------------------------
/**
*/
bool
Clipboard::Paste(const Ptr<SceneDocument>& doc, const Util::Guid& collection, Game::Entity parent, const char* action)
{
    if (this->core == nullptr || !doc.isvalid() || doc->entities.IsEmpty())
        return false;

    Ptr<PasteEntitiesCommand> command = PasteEntitiesCommand::Create();
    command->scene = this->core->GetScene();
    command->document = doc;
    command->collection = collection;
    command->parent = parent;
    command->action = action;

    this->core->GetCommandManager()->Execute(command);

    if (command->created.IsEmpty())
        return false;

    Util::String msg;
    msg.Format("%s %d entities", action, (int)command->created.Size());
    this->core->Log(LogMessageType::N_MESSAGE, msg);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
Clipboard::Duplicate(const Util::Array<Game::Entity>& entities, const Util::Guid& collection, Game::Entity parent)
{
    if (this->core == nullptr || entities.IsEmpty())
        return false;

    Util::Array<Util::Guid> guidByEntityIndex;
    Ptr<SceneDocument> doc = this->BuildDocument(this->core->GetScene().get(), entities, guidByEntityIndex);
    if (!doc.isvalid())
        return false;

    return this->Paste(doc, collection, parent, "Duplicate");
}

} // namespace Editor
