#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::SceneDocument

    A scene file as data: the collections, the entities and the prefab links exactly as they sit in
    the JSON, with component values as raw bytes. This is what persistence reads and writes, and
    what prefab instantiation works from, so the scene and the files on disk speak the same thing.

    A document never holds entity instances. Entity references inside component values are kept as
    the guid they were read from, and recorded in entityRefs so the scene can patch them once the
    entities exist. When writing, entity references are resolved through the guid array the scene
    passes in, indexed by the entity index in the authoring world.

    @see
    Editor::Scene

    @see
    Editor::Clipboard

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/componentid.h"
#include "core/refcounted.h"
#include "util/array.h"
#include "util/bitfield.h"
#include "util/blob.h"
#include "util/guid.h"
#include "util/string.h"
#include "util/stringatom.h"

namespace Editor
{

constexpr SizeT MAX_COMPONENT_FIELDS = 256;

//------------------------------------------------------------------------------
/**
    One component of a document entity. For a linked entity the value carries only the overridden
    fields, and every field that was not in the file sits at its component default.
*/
struct DocumentComponent
{
    Game::ComponentId component = Game::ComponentId::Invalid();
    /// raw component bytes, sized by the attribute registry
    Util::Blob value;
    /// which fields the file carried. Every field for an entity that is its own, and
    /// only the overridden ones for a linked entity. Read from the field names that
    /// were present on the node, which is what the merge uses to tell an overridden
    /// field from one that sits at its default
    Util::BitField<MAX_COMPONENT_FIELDS> presentFields;
    /// a component the instance has and its prefab chain does not
    bool addedToPrefab = false;
    /// a component the prefab chain has and the instance removed. No value is written
    /// for it, only the marker
    bool removedFromPrefab = false;
};

//------------------------------------------------------------------------------
/**
    One feature's data for a document entity. The document holds it as the text the file carried,
    and never interprets it, which is what lets a feature serialize something the editor has no type
    knowledge of. The text is kept even when no extension is registered for the name, so a level
    written by a build that has an extension still saves from a build that does not.

    @see
    Game::LevelExtension
*/
struct DocumentExtension
{
    /// the extension the node belongs to
    Util::StringAtom name;
    /// the json text exactly as the file carried it
    Util::String json;
    /// the keys the text carries, as the extension names them. Read from the text rather than from
    /// the
    /// scene, because the scene cannot see inside it, and this is what tells an authored field from
    /// one
    /// the entity accepts from its prefab chain
    Util::Array<Util::String> fields;
};

//------------------------------------------------------------------------------
/**
    An entity as it sits in a scene file. The components are the full value for an entity that is
    its own, and the overrides for an entity that is an instance of an entity in another file.
*/
struct DocumentEntity
{
    /// the guid of this entity in this document. Entity references in the file point at it
    Util::Guid guid;
    /// display name
    Util::String name;
    /// the collection the entity is authored in. Invalid means directly under the scene root
    Util::Guid collection;
    /// the entity is one this instance has and its prefab chain does not. There is no record for it
    /// in the file, so the node has no counterpart to name, and the file gains a record for it when
    /// the instance is applied to the prefab
    bool addedToPrefab = false;
    /// the entity is one the prefab chain has and this instance does not have. No value is written
    /// for it, only the marker, and the node is what carries it
    bool removedFromPrefab = false;
    /// the instantiation this entity belongs to. Invalid for an entity that is its own
    Util::Guid instance;
    /// the instantiation that instantiated this entity's prefab. Invalid for a top level instance
    Util::Guid parentInstance;
    /// the scene file this entity instantiates. Empty for an entity that is its own
    Util::String prefabSource;
    /// the guid of this entity's counterpart in prefabSource
    Util::Guid prefabEntity;
    /// the components, with their values. Only an entity that is its own carries full values
    Util::Array<DocumentComponent> components;
    /// the feature data, in the order the extensions were registered
    Util::Array<DocumentExtension> extensions;
};

//------------------------------------------------------------------------------
/**
    An authored collection as it sits in a scene file.
*/
struct DocumentCollection
{
    Util::Guid guid;
    Util::String name;
    /// the collection this one is nested in. Invalid means a root collection
    Util::Guid parent;
    /// position among its siblings
    IndexT order = 0;
};

//------------------------------------------------------------------------------
/**
    An entity reference that was read out of a component value. The field is named rather than
    offset so the scene can find it in the component's reflection, and the target is the guid the
    file pointed at.
*/
struct DocumentEntityRef
{
    /// index of the DocumentEntity the value belongs to
    IndexT entity;
    Game::ComponentId component = Game::ComponentId::Invalid();
    /// the field name the reference sits in
    Util::String field;
    /// the guid the file pointed at
    Util::Guid target;
};

//------------------------------------------------------------------------------
/**
    The whole contents of one scene file.
*/
class SceneDocument : public Core::RefCounted
{
    __DeclareClass(SceneDocument)
public:
    /// read a scene file into a document. Null when the file cannot be read or parsed
    static Ptr<SceneDocument> FromFile(const Util::String& path);
    /// read a json string in the scene file format into a document. Null when the string cannot be
    /// parsed. This is what the clipboard carries, and it is the same format as a scene file, so a
    /// string copied in the editor can be pasted into a file and a file can be pasted into the
    /// editor
    static Ptr<SceneDocument> FromJson(const Util::String& json);

    /// write the document to a scene file. Entity references in component values are written
    /// as the guid at the entity's index in guidByEntityIndex. False when the file cannot
    /// be opened
    bool ToFile(const Util::String& path, const Util::Array<Util::Guid>& guidByEntityIndex) const;
    /// the same document as a json string, which is what the clipboard holds. Empty when the
    /// document cannot be written
    Util::String ToJson(const Util::Array<Util::Guid>& guidByEntityIndex) const;

    /// the index of an entity by its guid in this document, or InvalidIndex
    IndexT FindEntity(const Util::Guid& guid) const;
    /// the index of a component on a document entity, or InvalidIndex
    IndexT FindComponent(IndexT entity, Game::ComponentId component) const;
    /// the index of an extension entry on a document entity, or InvalidIndex
    IndexT FindExtension(IndexT entity, Util::StringAtom name) const;
    /// the index of a collection by its guid, or InvalidIndex
    IndexT FindCollection(const Util::Guid& guid) const;

    /// the uri the document was read from, and what a save writes back when it is not overridden
    Util::String source;
    /// collections in the order they were read
    Util::Array<DocumentCollection> collections;
    /// entities in the order they were read
    Util::Array<DocumentEntity> entities;
    /// the entity references read out of the component values, patched by the scene
    Util::Array<DocumentEntityRef> entityRefs;
};

} // namespace Editor
