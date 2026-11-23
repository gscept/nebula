//------------------------------------------------------------------------------
//  @file scenedocument.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/model/scenedocument.h"
#include "game/component.h"
#include "game/componentserialization.h"
#include "game/entity.h"
#include "game/levelextension.h"
#include "io/ioserver.h"
#include "io/jsonreader.h"
#include "io/jsonwriter.h"
#include "io/memorystream.h"
#include "io/stream.h"
#include "io/uri.h"
#include "memdb/attributeregistry.h"
#include "util/bitfield.h"
#include "util/stringatom.h"

namespace Editor
{

__ImplementClass(Editor::SceneDocument, 'SCDM', Core::RefCounted);

// The level format this writes. 100 is the level format the LevelParser reads, which is
// read as well but never written: a saved scene is always the fuller format
static const int WrittenLevelVersion = 101;
static const int OldestReadableLevelVersion = 100;

//------------------------------------------------------------------------------
/**
    A component whose interface is not registered, which is what an entity written by a newer build
    than this one carries. The value is skipped and named, which is what the LevelParser does with
    components it cannot resolve.
*/
static void
WarnUnknownComponent(const Util::String& entityName, const Util::String& componentName)
{
    n_warning("Scene file entity '%s' has unknown component '%s'\n", entityName.AsCharPtr(), componentName.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
    The interface of a registered component, or null when it is not one.
*/
static Game::ComponentInterface*
GetComponentInterface(Game::ComponentId component)
{
    if (component == Game::ComponentId::Invalid())
        return nullptr;
    return static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(component));
}

//------------------------------------------------------------------------------
/**
    Write one field of a component value by its reflected type name. These are the types the
    component descriptions can name, which is the same set the inspector knows. An entity reference
    goes through the writer's Game::Entity specialization, which is where the type override turns it
    back into a guid.

    @todo
    This is hardcoded, and we can't support enums and custom types and similar automatically
    with this approach. Should reconsider later.
*/
static void
WriteField(IO::JsonWriter* writer, const char* typeName, const void* field, const Util::String& name)
{
    const Util::StringAtom type(typeName);

    if (type == "bool"_atm)
        writer->Add<bool>(*(const bool*)field, name);
    else if (type == "int"_atm)
        writer->Add<int>(*(const int*)field, name);
    else if (type == "uint"_atm)
        writer->Add<unsigned int>(*(const unsigned int*)field, name);
    else if (type == "int32_t"_atm)
        writer->Add<int>(*(const int*)field, name);
    else if (type == "uint32_t"_atm)
        writer->Add<unsigned int>(*(const unsigned int*)field, name);
    else if (type == "int16_t"_atm)
        writer->Add<int16_t>(*(const int16_t*)field, name);
    else if (type == "uint16_t"_atm)
        writer->Add<uint16_t>(*(const uint16_t*)field, name);
    else if (type == "uint64_t"_atm)
        writer->Add<uint64_t>(*(const uint64_t*)field, name);
    else if (type == "float"_atm)
        writer->Add<float>(*(const float*)field, name);
    else if (type == "Math::vec2"_atm)
        writer->Add<Math::vec2>(*(const Math::vec2*)field, name);
    else if (type == "Math::vec3"_atm)
        writer->Add<Math::vec3>(*(const Math::vec3*)field, name);
    else if (type == "Math::vec4"_atm)
        writer->Add<Math::vec4>(*(const Math::vec4*)field, name);
    else if (type == "Math::quat"_atm)
        writer->Add<Math::quat>(*(const Math::quat*)field, name);
    else if (type == "Math::mat4"_atm)
        writer->Add<Math::mat4>(*(const Math::mat4*)field, name);
    else if (type == "Util::StringAtom"_atm)
        writer->Add<Util::StringAtom>(*(const Util::StringAtom*)field, name);
    else if (type == "Util::BitField<16>"_atm)
        writer->Add(*(const Util::BitField<16>*)field, name);
    else if (type == "Util::BitField<32>"_atm)
        writer->Add(*(const Util::BitField<32>*)field, name);
    else if (type == "Util::BitField<64>"_atm)
        writer->Add(*(const Util::BitField<64>*)field, name);
    else if (type == "Game::Entity"_atm)
        writer->Add<Game::Entity>(*(const Game::Entity*)field, name);
    else
        n_warning("Scene file writer does not know how to write field '%s' of type '%s'\n", name.AsCharPtr(), typeName);
}

// The parse of one file reads its entity references through this context, which is
// what turns the guid in the file into a pending reference instead of an entity value.
struct SceneParseContext
{
    Editor::SceneDocument* doc = nullptr;
    IndexT currentEntity = InvalidIndex;
    Game::ComponentId currentComponent = Game::ComponentId::Invalid();
};

//------------------------------------------------------------------------------
/**
    Reading a scene file goes through the same component serialization the LevelParser uses, so a
    scene file and a level file stay one format. The difference is the entity type override: the
    LevelParser resolves a guid against entities that already exist, while a document has none yet,
    so the reference is kept as a pending reference the scene patches once the entities are there.
*/
static Ptr<SceneDocument>
ParseLevel(IO::JsonReader* reader)
{
    reader->SetToRoot();
    if (!reader->SetToNode("/level"))
    {
        n_warning("Scene json has no level root\n");
        return nullptr;
    }

    const int version = reader->GetInt("version");
    if (version < OldestReadableLevelVersion || version > WrittenLevelVersion)
    {
        n_warning("Scene json has unsupported version %d\n", version);
        return nullptr;
    }

    Ptr<SceneDocument> doc = SceneDocument::Create();

    SceneParseContext ctx;
    ctx.doc = doc.get();

    Game::ComponentSerialization::OverrideType(
        Game::ComponentSerialization::ENTITY,
        [&ctx](const Ptr<IO::JsonReader>& reader, const char* name, void* data)
        {
            DocumentEntityRef ref;
            reader->Get<Util::Guid>(ref.target, name);
            ref.entity = ctx.currentEntity;
            ref.component = ctx.currentComponent;
            ref.field = name;
            ctx.doc->entityRefs.Append(ref);
            *(Game::Entity*)data = Game::Entity::Invalid();
        },
        nullptr);

    if (reader->HasAttr("collections") && reader->SetToFirstChild("collections"))
    {
        // Indexed, because SetToNextChild takes the cursor back to the parent when it runs
        // out of siblings, which is a pop the following reads would have to account for
        const IndexT numCollections = (IndexT)reader->CurrentSize();
        for (IndexT collectionIndex = 0; collectionIndex < numCollections; collectionIndex++)
        {
            const Util::String key = reader->GetChildNodeName(collectionIndex);

            DocumentCollection collection;
            collection.guid = Util::Guid::FromString(key);

            if (reader->SetToNode(key))
            {
                collection.name = reader->GetOptString("name", "collection");
                if (reader->HasAttr("parent"))
                    reader->Get<Util::Guid>(collection.parent, "parent");
                reader->SetToParent();
            }

            collection.order = (IndexT)doc->collections.Size();
            doc->collections.Append(collection);
        }
        reader->SetToParent();
    }

    if (reader->SetToFirstChild("entities") && reader->SetToFirstChild())
    {
        do
        {
            DocumentEntity entity;
            entity.guid = Util::Guid::FromString(reader->GetCurrentNodeName());
            entity.name = reader->GetOptString("name", "unnamed_entity");

            if (reader->HasAttr("collection"))
                reader->Get<Util::Guid>(entity.collection, "collection");

            // Read before the link block, because the cursor goes into it and the attributes of the
            // entity node are only reachable before that
            entity.addedToPrefab = reader->GetOptBool("addedToPrefab", false);
            entity.removedFromPrefab = reader->GetOptBool("removedFromPrefab", false);

            const IndexT docIndex = doc->entities.Size();
            ctx.currentEntity = docIndex;

            if (reader->HasAttr("prefab") && reader->SetToFirstChild("prefab"))
            {
                if (reader->HasAttr("source"))
                    entity.prefabSource = reader->GetString("source");
                if (reader->HasAttr("entity"))
                    reader->Get<Util::Guid>(entity.prefabEntity, "entity");
                if (reader->HasAttr("instance"))
                    reader->Get<Util::Guid>(entity.instance, "instance");
                if (reader->HasAttr("parentInstance"))
                    reader->Get<Util::Guid>(entity.parentInstance, "parentInstance");

                // The overrides sit under the link, because they say what this entity changed
                // about the file it links into
                if (reader->HasAttr("overrides") && reader->SetToFirstChild("overrides"))
                {
                    const IndexT numComponents = (IndexT)reader->CurrentSize();
                    for (IndexT componentIndex = 0; componentIndex < numComponents; componentIndex++)
                    {
                        const Util::String componentName = reader->GetChildNodeName(componentIndex);
                        const Game::ComponentId componentId = Game::GetComponentId(componentName);
                        Game::ComponentInterface* interface = GetComponentInterface(componentId);
                        if (interface == nullptr)
                        {
                            WarnUnknownComponent(entity.name, componentName);
                            continue;
                        }

                        ctx.currentComponent = componentId;

                        DocumentComponent component;
                        component.component = componentId;

                        // The markers and the field names live on the component node. The generated
                        // reader ignores attributes it does not know, so the markers can sit next
                        // to
                        // the fields. A field the file did not carry stays at its default and is
                        // not
                        // marked present, which is what tells an overridden field from one the
                        // instance accepts from the prefab
                        if (reader->SetToNode(componentName))
                        {
                            component.addedToPrefab = reader->GetOptBool("addedToPrefab", false);
                            component.removedFromPrefab = reader->GetOptBool("removedFromPrefab", false);
                            if (!component.removedFromPrefab)
                            {
                                for (IndexT field = 0; field < (IndexT)interface->GetNumFields(); field++)
                                {
                                    if (reader->HasAttr(interface->GetFieldNames()[field]))
                                        component.presentFields.SetBit((uint64_t)field);
                                }
                            }
                            reader->SetToParent();
                        }

                        const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(componentId);
                        if (typeSize > 0 && !component.removedFromPrefab)
                        {
                            Util::Blob value(MemDb::AttributeRegistry::DefaultValue(componentId), typeSize);
                            Game::ComponentSerialization::Deserialize(reader, componentId, value.GetPtr());
                            component.value = value;
                        }

                        entity.components.Append(component);
                    }
                    reader->SetToParent();
                }
                reader->SetToParent();
            }
            else if (reader->SetToFirstChild("components"))
            {
                const IndexT numComponents = (IndexT)reader->CurrentSize();
                for (IndexT componentIndex = 0; componentIndex < numComponents; componentIndex++)
                {
                    const Util::String componentName = reader->GetChildNodeName(componentIndex);
                    const Game::ComponentId componentId = Game::GetComponentId(componentName);
                    if (componentId == Game::ComponentId::Invalid())
                    {
                        WarnUnknownComponent(entity.name, componentName);
                        continue;
                    }

                    ctx.currentComponent = componentId;

                    DocumentComponent component;
                    component.component = componentId;
                    for (IndexT field = 0; field < MAX_COMPONENT_FIELDS; field++)
                        component.presentFields.SetBit((uint64_t)field);

                    const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(componentId);
                    if (typeSize > 0)
                    {
                        Util::Blob value(MemDb::AttributeRegistry::DefaultValue(componentId), typeSize);
                        Game::ComponentSerialization::Deserialize(reader, componentId, value.GetPtr());
                        component.value = value;
                    }

                    entity.components.Append(component);
                }
                reader->SetToParent();
            }

            // The feature data is read as text and never interpreted, so an extension this build
            // does
            // not have still keeps what the file carried for it, and a save writes it back
            // unchanged
            if (reader->SetToFirstChild("extensions"))
            {
                // Indexed, because SetToNextChild takes the cursor back to the parent when it runs
                // out of siblings, which is a pop the following reads would have to account for
                const IndexT numExtensions = (IndexT)reader->CurrentSize();
                for (IndexT extensionIndex = 0; extensionIndex < numExtensions; extensionIndex++)
                {
                    const Util::String key = reader->GetChildNodeName(extensionIndex);

                    DocumentExtension extension;
                    extension.name = Util::StringAtom(key);

                    if (reader->SetToNode(key))
                    {
                        extension.json = reader->GetRawJson();
                        reader->SetToParent();
                    }

                    // Which keys the text carries is something the extension knows and the document
                    // does not,
                    // so the extension is asked about its own text. An extension this build does
                    // not have
                    // cannot answer, and its entry keeps no keys, which is what a save writes back
                    // whole
                    const Game::LevelExtension* extensionHook = Game::LevelExtensionRegistry::GetExtension(extension.name);
                    if (extensionHook != nullptr && extensionHook->keys != nullptr && !extension.json.IsEmpty())
                    {
                        extensionHook->keys(extension.json.AsCharPtr(), extension.fields);
                    }

                    // Which entities the text names is something only the extension can say, because the
                    // document reads the text as bytes. This is what makes a reference inside extension
                    // data a reference the document knows, the same way a component field's reference is,
                    // and it is what lets the writer put this file's guids into it
                    if (extensionHook != nullptr && extensionHook->refs != nullptr && !extension.json.IsEmpty())
                    {
                        Util::Array<Game::TextRef> named;
                        extensionHook->refs(extension.json.AsCharPtr(), named);
                        for (IndexT r = 0; r < named.Size(); r++)
                        {
                            DocumentEntityRef ref;
                            ref.entity = docIndex;
                            ref.extension = extension.name;
                            ref.field = named[r].path;
                            ref.target = named[r].guid;
                            doc->entityRefs.Append(ref);
                        }
                    }

                    entity.extensions.Append(extension);
                }
                reader->SetToParent();
            }

            doc->entities.Append(entity);
        } while (reader->SetToNextChild());
        reader->SetToParent();
    }

    Game::ComponentSerialization::OverrideType(Game::ComponentSerialization::ENTITY, nullptr, nullptr);

    return doc;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<SceneDocument>
SceneDocument::FromFile(const Util::String& path)
{
    const IO::URI uri(path);
    if (uri.Scheme() != "file")
    {
        n_warning("Scene file '%s' is not a file path\n", path.AsCharPtr());
        return nullptr;
    }

    Ptr<IO::Stream> stream = IO::CreateStream(uri);
    if (!stream.isvalid())
    {
        n_warning("Could not open scene file '%s'\n", path.AsCharPtr());
        return nullptr;
    }
    stream->SetAccessMode(IO::Stream::ReadAccess);

    Ptr<IO::JsonReader> reader = IO::JsonReader::Create();
    reader->SetStream(stream);
    if (!reader->Open())
    {
        n_warning("Could not read scene file '%s' as json\n", path.AsCharPtr());
        if (stream->IsOpen())
            stream->Close();
        return nullptr;
    }

    Ptr<SceneDocument> doc = ParseLevel(reader.get());
    reader->Close();

    if (doc.isvalid())
        doc->source = path;
    return doc;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<SceneDocument>
SceneDocument::FromJson(const Util::String& json)
{
    if (json.IsEmpty())
        return nullptr;
    if (!json.BeginsWithString("{"))
    {
        return nullptr;
    }

    Ptr<IO::MemoryStream> stream = IO::MemoryStream::Create();
    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (!stream->Open())
        return nullptr;
    stream->Write(json.AsCharPtr(), json.Length());
    stream->Close();

    stream->SetAccessMode(IO::Stream::ReadAccess);

    Ptr<IO::JsonReader> reader = IO::JsonReader::Create();
    reader->SetStream(stream);
    if (!reader->Open())
        return nullptr;

    Ptr<SceneDocument> doc = ParseLevel(reader.get());
    reader->Close();
    return doc;
}

//------------------------------------------------------------------------------
/**
    Writing goes through the same component serialization, and the entity type override is put the
    other way: an entity value in a component is written as the guid at the entity's index in the
    array the scene passed in, which is what the reader's override resolves back into an entity.
*/
static void
WriteLevel(const SceneDocument* doc, IO::JsonWriter* writer, const Util::Array<Util::Guid>& guidByEntityIndex)
{
    // A plain component node is the component's whole value, and an entity reference that is empty
    // is what
    // the component defaults to, so there is nothing to say about it. Under the overrides the field
    // is in
    // the file because the instance said it, so an empty one is written. This is what the entity
    // handler
    // below reads
    bool writeEmptyEntityRefs = true;

    Game::ComponentSerialization::OverrideType(
        Game::ComponentSerialization::ENTITY,
        nullptr,
        [&guidByEntityIndex, &writeEmptyEntityRefs](const Ptr<IO::JsonWriter>& writer, const char* name, void* data)
        {
            const Game::Entity entity = *(Game::Entity*)data;
            if (entity == Game::Entity::Invalid() && !writeEmptyEntityRefs)
                return;

            Util::Guid guid;
            if (entity != Game::Entity::Invalid() && entity.index < guidByEntityIndex.Size())
                guid = guidByEntityIndex[entity.index];
            writer->Add<Util::Guid>(guid, Util::String(name));
        });

    writer->BeginObject("level");
    writer->Add(WrittenLevelVersion, "version");

    writer->BeginObject("collections");
    for (IndexT i = 0; i < doc->collections.Size(); i++)
    {
        const DocumentCollection& collection = doc->collections[i];
        writer->BeginObject(collection.guid.AsString().AsCharPtr());
        writer->Add(collection.name, "name");
        if (collection.parent.IsValid())
            writer->Add<Util::Guid>(collection.parent, "parent");
        writer->Add((int)collection.order, "order");
        writer->End();
    }
    writer->End();

    writer->BeginObject("entities");
    for (IndexT i = 0; i < doc->entities.Size(); i++)
    {
        const DocumentEntity& entity = doc->entities[i];

        writer->BeginObject(entity.guid.AsString().AsCharPtr());
        writer->Add(entity.name, "name");
        if (entity.collection.IsValid())
            writer->Add<Util::Guid>(entity.collection, "collection");
        if (entity.addedToPrefab)
            writer->Add(true, "addedToPrefab");
        if (entity.removedFromPrefab)
            writer->Add(true, "removedFromPrefab");

        if (!entity.prefabSource.IsEmpty())
        {
            writeEmptyEntityRefs = true;
            writer->BeginObject("prefab");
            writer->Add(entity.prefabSource, "source");
            if (!entity.addedToPrefab)
                writer->Add<Util::Guid>(entity.prefabEntity, "entity");
            if (entity.instance.IsValid())
                writer->Add<Util::Guid>(entity.instance, "instance");
            if (entity.parentInstance.IsValid())
                writer->Add<Util::Guid>(entity.parentInstance, "parentInstance");

            // An instance entity that changed nothing about its chain has nothing to say
            // about it either, which is what keeps a level that only moved an instance around
            // from carrying a block of empty overrides
            if (!entity.components.IsEmpty())
            {
                writer->BeginObject("overrides");
                for (IndexT c = 0; c < entity.components.Size(); c++)
                {
                    const DocumentComponent& component = entity.components[c];
                    Game::ComponentInterface* interface = GetComponentInterface(component.component);
                    if (interface == nullptr)
                        continue;

                    writer->BeginObject(interface->name.Value());

                    if (component.removedFromPrefab)
                    {
                        writer->Add(true, "removedFromPrefab");
                    }
                    else
                    {
                        if (component.addedToPrefab)
                            writer->Add(true, "addedToPrefab");

                        const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(component.component);
                        if (typeSize > 0 && component.value.IsValid())
                        {
                            const byte* base = (const byte*)component.value.GetPtr();
                            const IndexT numFields = (IndexT)interface->GetNumFields();
                            for (IndexT field = 0; field < numFields; field++)
                            {
                                if (!component.presentFields.IsSet((uint64_t)field))
                                    continue;

                                WriteField(writer, interface->GetFieldTypenames()[field],
                                    base + interface->GetFieldByteOffsets()[field],
                                    Util::String(interface->GetFieldNames()[field]));
                            }
                        }
                    }

                    writer->End();
                }
                writer->End();
            }

            writer->End();
        }
        else
        {
            writeEmptyEntityRefs = false;
            writer->BeginObject("components");
            for (IndexT c = 0; c < entity.components.Size(); c++)
            {
                const DocumentComponent& component = entity.components[c];
                if (component.component == Game::ComponentId::Invalid())
                    continue;

                // A component with no fields writes an empty object, and its serializer never
                // reads the pointer, so it goes through the same call
                void* value = component.value.IsValid() ? component.value.GetPtr() : nullptr;
                Game::ComponentSerialization::Serialize(writer, component.component, value);
            }
            writer->End();
        }

        // The feature data is written as the text it was read as, or as the text the feature
        // produced
        // for the entity. The writer has no type knowledge of it, which is what keeps an extension
        // this
        // build does not have written back as the text it came in as
        if (!entity.extensions.IsEmpty())
        {
            writer->BeginObject("extensions");
            for (IndexT e = 0; e < entity.extensions.Size(); e++)
            {
                const DocumentExtension& extension = entity.extensions[e];
                if (extension.json.IsEmpty())
                    continue;

                // A text that names entities names them by the guids of the file it was read from, and
                // this file can only name its own records. The writer cannot read the text, so it asks the
                // extension that wrote it for the text with this file's guids put back. A reference this
                // file has no record for is given with no guid, which is what the extension writes as
                // nothing, and an extension that cannot answer for its own text is written whole
                const Game::LevelExtension* extensionHook = Game::LevelExtensionRegistry::GetExtension(extension.name);
                if (extensionHook == nullptr || extensionHook->remap == nullptr)
                {
                    writer->AddRaw(extension.json.AsCharPtr(), Util::String(extension.name.Value()));
                    continue;
                }

                Util::String pairs;
                for (IndexT r = 0; r < doc->entityRefs.Size(); r++)
                {
                    const DocumentEntityRef& ref = doc->entityRefs[r];
                    if (ref.entity != i || ref.extension != extension.name)
                        continue;

                    const IndexT targetIndex = doc->FindEntity(ref.target);
                    if (targetIndex == InvalidIndex)
                    {
                        n_warning("Scene file entity '%s' has extension reference '%s' pointing at a guid "
                            "that is not in that file\n", entity.name.AsCharPtr(), ref.field.AsCharPtr());
                    }

                    if (!pairs.IsEmpty())
                        pairs += "\n";
                    pairs += ref.field;
                    pairs += "\t";
                    if (targetIndex != InvalidIndex)
                        pairs += guidByEntityIndex[targetIndex].AsString();
                }

                if (pairs.IsEmpty())
                {
                    writer->AddRaw(extension.json.AsCharPtr(), Util::String(extension.name.Value()));
                    continue;
                }

                Util::String text;
                extensionHook->remap(extension.json.AsCharPtr(), pairs.AsCharPtr(), text);
                writer->AddRaw(text.AsCharPtr(), Util::String(extension.name.Value()));
            }
            writer->End();
        }

        writer->End();
    }
    writer->End();

    writer->End();

    Game::ComponentSerialization::OverrideType(Game::ComponentSerialization::ENTITY, nullptr, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
Util::String
SceneDocument::ToJson(const Util::Array<Util::Guid>& guidByEntityIndex) const
{
    Ptr<IO::MemoryStream> stream = IO::MemoryStream::Create();
    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (!stream->Open())
        return Util::String();

    Ptr<IO::JsonWriter> writer = IO::JsonWriter::Create();
    writer->SetStream(stream);
    if (!writer->Open())
    {
        if (stream->IsOpen())
            stream->Close();
        return Util::String();
    }

    WriteLevel(this, writer.get(), guidByEntityIndex);

    // The document is serialized into the stream when the writer closes, which is what
    // makes the stream the string
    writer->Close();

    if (stream->GetSize() == 0)
        return Util::String();

    return Util::String((const char*)stream->GetRawPointer(), (size_t)stream->GetSize());
}

//------------------------------------------------------------------------------
/**
*/
bool
SceneDocument::ToFile(const Util::String& path, const Util::Array<Util::Guid>& guidByEntityIndex) const
{
    const Util::String json = this->ToJson(guidByEntityIndex);
    if (json.IsEmpty())
        return false;

    IO::URI uri(path);
    IO::EnsureDirectoriesForFile(uri);

    Ptr<IO::Stream> stream = IO::CreateStream(uri);
    if (!stream.isvalid())
    {
        n_warning("Could not open scene file '%s' for writing\n", path.AsCharPtr());
        return false;
    }
    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (!stream->Open())
    {
        n_warning("Could not write scene file '%s'\n", path.AsCharPtr());
        return false;
    }

    stream->Write(json.AsCharPtr(), json.Length());
    stream->Close();
    return true;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
SceneDocument::FindEntity(const Util::Guid& guid) const
{
    if (!guid.IsValid())
        return InvalidIndex;

    for (IndexT i = 0; i < this->entities.Size(); i++)
    {
        if (this->entities[i].guid == guid)
            return i;
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
SceneDocument::FindComponent(IndexT entity, Game::ComponentId component) const
{
    if (entity == InvalidIndex || entity >= this->entities.Size())
        return InvalidIndex;

    const Util::Array<DocumentComponent>& components = this->entities[entity].components;
    for (IndexT i = 0; i < components.Size(); i++)
    {
        if (components[i].component == component)
            return i;
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
SceneDocument::FindExtension(IndexT entity, Util::StringAtom name) const
{
    if (entity == InvalidIndex || entity >= this->entities.Size())
        return InvalidIndex;

    const Util::Array<DocumentExtension>& extensions = this->entities[entity].extensions;
    for (IndexT i = 0; i < extensions.Size(); i++)
    {
        if (extensions[i].name == name)
            return i;
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
SceneDocument::FindCollection(const Util::Guid& guid) const
{
    if (!guid.IsValid())
        return InvalidIndex;

    for (IndexT i = 0; i < this->collections.Size(); i++)
    {
        if (this->collections[i].guid == guid)
            return i;
    }
    return InvalidIndex;
}

} // namespace Editor
