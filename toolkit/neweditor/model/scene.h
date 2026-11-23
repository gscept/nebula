#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::Scene

    One editor tab. The scene holds what the user has authored. The scene has its own world for the
    authored data, and that world is never simulated.

    Every change is written to it and then copied to the sim world, which is the world the
    processors run in and the viewport shows. The sim world is shared by all open scenes, so only
    the active scene is in it.

    Selection, the undo history and the view belong to the scene, so a tab you come back to has them
    still. Play runs the sim world. Stopping play throws it away and copies the authored data back
    over it.

    Entities are grouped in collections, and the scene owns that grouping as well.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "basegamefeature/components/basegamefeature.h"
#include "core/ptr.h"
#include "core/refcounted.h"
#include "neweditor/model/editcommandmanager.h"
#include "neweditor/viewtools/editorcamera.h"
#include "neweditor/model/scenedocument.h"
#include "neweditor/model/selectioncontext.h"
#include "game/componentid.h"
#include "game/entity.h"
#include "game/levelextension.h"
#include "game/world.h"
#include "math/quat.h"
#include "math/vec3.h"
#include "util/array.h"
#include "util/bitfield.h"
#include "util/dictionary.h"
#include "util/guid.h"
#include "util/hashtable.h"
#include "util/string.h"

namespace Editor
{

class Shell;

//------------------------------------------------------------------------------
/**
    What an instance wrote on top of its prefab file. An entry covers a component, or one
    extension's data, which the scene cannot read.
*/
struct PrefabOverride
{
    Game::ComponentId component = Game::ComponentId::Invalid();
    /// the fields the instance wrote, by reflected field index
    Util::BitField<MAX_COMPONENT_FIELDS> overriddenFields;
    /// the extension the entry is for. Invalid for a component entry
    Util::StringAtom extension;
    /// the keys the instance wrote for that extension, named the way the extension names them
    Util::Array<Util::String> fields;
    /// the instance has this component and the prefab file does not. A revert takes it away
    bool componentAdded = false;
    /// the prefab file has this component and the instance removed it. A revert puts it back
    bool componentRemoved = false;
};

//------------------------------------------------------------------------------
/**
    A record in the prefab file that this instance has no entity for. It is what a save writes as an
    empty node.
*/
struct RemovedRecord
{
    /// the record in the prefab file
    Util::Guid record;
    /// the name the record has in the file, kept so a save does not have to read the file
    Util::String name;
    /// the collection the entity was in, so an undo puts it back there
    Util::Guid collection;
};

//------------------------------------------------------------------------------
/**
    One instantiation of a prefab file in this scene. Every entity it created points back here
    through its editable record.
*/
struct PrefabInstance
{
    /// false once the instance has been unpacked. The slot stays in the array, because the records
    /// and the
    /// saved files use its index
    bool valid = false;
    /// the identity of this instantiation within the scene
    Util::Guid guid;
    /// the prefab file this instance was made from, and the start of the chain its entities walk
    Util::String source;
    /// every entity this instance created
    Util::Array<Game::Entity> entities;
    /// the records of the file that have no entity for them
    Util::Array<RemovedRecord> removedRecords;
    /// the collections this instance created from the file's collection nodes. They stay as
    /// ordinary
    /// collections afterwards
    Util::Array<Util::Guid> collections;
};

//------------------------------------------------------------------------------
/**
    One record of a prefab chain: the file it is in and the name it has there. An empty source means
    this scene wrote the value itself.
*/
struct ChainRecord
{
    /// the scene file the record belongs to
    Util::String source;
    /// the name the record has in that file, which is not always the name the level gave it
    Util::String record;
};

//------------------------------------------------------------------------------
/**
    The identity of an authored entity, indexed by the entity index in the authoring world.
*/
struct Editable
{
    /// persistent identity
    Util::Guid guid;
    /// display name
    Util::String name;
    /// the entity in the authoring world
    Game::Entity entity = Game::Entity::Invalid();
    /// the same entity in the sim world. Invalid if it has not been copied there yet
    Game::Entity gameEntity = Game::Entity::Invalid();
    /// the collection the entity is in. Invalid means it is under the scene root
    Util::Guid collection;
    /// the prefab instance that created this entity, as an index into the scene's instances.
    /// Invalid when the
    /// entity is its own
    IndexT prefabInstance = InvalidIndex;
    /// this entity's record in the instance's prefab file, which is where its chain begins
    Util::Guid prefabEntity;
    /// the fields this entity wrote over its prefab chain, and the components it added or removed
    Util::Array<PrefabOverride> prefabOverrides;
};

//------------------------------------------------------------------------------
/**
    An authored collection.
*/
struct Collection
{
    /// persistent identity
    Util::Guid guid;
    /// display name
    Util::String name;
    /// the collection this one is inside. Invalid means it is a root collection
    Util::Guid parent;
    /// position among its siblings, in the order they were created
    IndexT order = 0;
};

class Scene : public Core::RefCounted
{
    __DeclareClass(Scene)
public:
    Scene();
    virtual ~Scene();

    /// create the authoring world, the selection and the undo stacks. The sim world is the game's
    void Setup(Shell* core, const Util::String& sceneName);
    /// destroy the authoring world and the history
    void Discard();

    /// the scene's identity, which tabs and panel state are keyed by
    const Util::Guid& GetGuid() const;
    /// the name of the scene
    const Util::String& GetName() const;
    /// the world that holds the authored data
    Game::World* GetAuthoringWorld() const;
    /// the world the processors run in. Shared by every open scene
    Game::World* GetSimWorld() const;

    /// clear the sim world and copy this scene's data into it. This is what a tab switch does
    void Activate();
    /// the scene is no longer shown, so its sim data is gone
    void Deactivate();

    /// the scene's own view. The camera object it writes to is the shared editor camera
    EditorCamera& GetCamera();
    /// put the view back at the starting one
    void ResetCamera();

    /// the scene's own selection
    Ptr<SelectionContext> GetSelection() const;
    /// the scene's own undo and redo stacks
    Ptr<EditCommandManager> GetCommandManager() const;

    // -- Authoring. Everything here writes the authoring world and copies the change to the sim
    // world --

    /// create an authored entity
    Game::Entity CreateEntity(const Util::String& entityName);
    /// delete an authored entity, instance and id
    void DeleteEntity(Game::Entity entity);

    /// allocate an entity id without an instance, so a command can hold it across an undo
    Game::Entity AllocateEntityId();
    /// release an id that has no instance
    void DeallocateEntityId(Game::Entity entity);
    /// create the instance of an entity from a record that already has its id
    void AllocateInstance(const Editable& record);
    /// remove an instance and keep the id, which is what an undo of a create does
    void DeallocateInstance(Game::Entity entity);

    /// add a component to an entity and return its data. A null value is a plain add, a value is
    /// what an
    /// undo of a removal puts back
    void* AddComponent(Game::Entity entity, Game::ComponentId component, const void* value);
    /// remove a component from an entity
    void RemoveComponent(Game::Entity entity, Game::ComponentId component);
    /// set a component value
    void SetComponent(Game::Entity entity, Game::ComponentId component, const void* value);

    /// write a value into the sim world only, without touching the authored value. This is what a
    /// gizmo drag
    /// shows while it runs
    void PreviewComponent(Game::Entity entity, Game::ComponentId component, const void* value);
    /// copy the authored value back into the sim world, which puts the view back after a dropped
    /// drag
    void RestoreComponent(Game::Entity entity, Game::ComponentId component);

    /// rename an entity
    bool SetEntityName(Game::Entity entity, const Util::String& name);
    /// a name no other entity has. The base is numbered until it does not collide
    Util::String UniqueEntityName(const Util::String& base) const;

    /// true when an entity has an instance. A command that only holds an id does not
    bool HasInstance(Game::Entity entity) const;

    /// the components an entity has, in the order of its table
    const Util::Array<Game::ComponentId>& GetComponents(Game::Entity entity);
    /// true for Position, Orientation and Scale
    static bool IsTransformComponent(Game::ComponentId component);
    /// pointer to the authored data of a component
    void* GetComponentData(Game::Entity entity, Game::ComponentId component);

    /// the editable record of an entity
    const Editable& GetEditable(Game::Entity entity) const;
    /// all editable records, indexed by the entity index in the authoring world
    const Util::Array<Editable>& GetEditables() const;
    /// bumped on every change, so a panel knows when it has to rebuild
    uint64_t GetVersion() const;
    /// how many entities the scene has
    IndexT NumEntities() const;
    /// the same entity in the sim world
    Game::Entity ToGameEntity(Game::Entity entity) const;
    /// the authored entity a sim world entity came from. Invalid if it was not authored
    Game::Entity ToEditorEntity(Game::Entity entity) const;

    // -- Collections --

    /// all collections, in the order they were created
    const Util::Array<Collection>& GetCollections() const;
    /// the index of a collection, or InvalidIndex when the guid is not one
    IndexT FindCollection(const Util::Guid& guid) const;
    /// create a collection under a parent. Invalid parent means the scene root
    Util::Guid CreateCollection(const Util::String& name, const Util::Guid& parent = Util::Guid());
    /// create a collection from a stored record, at the given index, which is what an undo of a
    /// delete puts
    /// it back at
    Util::Guid CreateCollection(const Collection& record, IndexT index = InvalidIndex);
    /// rename a collection
    bool SetCollectionName(const Util::Guid& guid, const Util::String& name);
    /// move a collection inside another one. Invalid parent makes it a root collection
    bool SetCollectionParent(const Util::Guid& guid, const Util::Guid& parent);
    /// delete a collection. Its child collections and entities move to its parent
    bool DeleteCollection(const Util::Guid& guid);
    /// move an entity into a collection
    void MoveEntityToCollection(Game::Entity entity, const Util::Guid& collection);
    /// the collection new entities are created in
    const Util::Guid& GetActiveCollection() const;
    /// set the collection new entities are created in
    void SetActiveCollection(const Util::Guid& guid);

    /// the parent of an entity, or Invalid when it has none
    Game::Entity GetParent(Game::Entity entity) const;
    /// make an entity a child of another. Adds Game::HTransform when the entity does not have it
    bool SetParent(Game::Entity entity, Game::Entity parent);
    /// clear the parent of an entity
    bool ClearParent(Game::Entity entity);

    // -- Persistence. A saved level and a saved prefab are the same kind of file --

    /// the file this scene was read from or last saved to. Empty for a scene that has never been
    /// saved
    const Util::String& GetFilePath() const;
    /// true when the data changed since the file was last written
    bool IsDirty() const;
    /// write the scene to a file and remember it as this scene's file
    bool Save(const Util::String& path);
    /// clear the scene and read a file into it: collections, entities, prefab instances and their
    /// values
    bool Load(const Util::String& path);

    // -- Prefabs --

    /// the scene's prefab instances
    const Util::Array<PrefabInstance>& GetPrefabInstances() const;
    /// true when an entity came from a prefab instance that is still linked
    bool IsPrefabInstanceEntity(Game::Entity entity) const;
    /// true when an entity is the root of its instantiation
    bool IsPrefabInstanceRoot(Game::Entity entity) const;
    /// the index of an entity's instance, or InvalidIndex when the entity is its own or its
    /// instance is gone
    IndexT GetPrefabInstanceIndex(Game::Entity entity) const;
    /// the instance an entity came from, or an empty entry
    const PrefabInstance& GetPrefabInstance(Game::Entity entity) const;
    /// every entity the same instance created
    void GetInstanceEntities(Game::Entity entity, Util::Array<Game::Entity>& out) const;
    /// read a prefab file and create its entities in a collection. The file's collection nodes
    /// become new
    /// collections, and the entities that name one go into it. Returns the entities created
    bool InstantiatePrefab(const Util::String& path, const Util::Guid& collection, Util::Array<Game::Entity>& created);

    /// put a document's entities into this scene, which is what a paste does. They get new guids
    /// and names,
    /// and the references inside the values point at the entities the paste made. The document's
    /// prefab links
    /// are not followed, because a paste is a copy and not an instance. Returns the entities
    /// created
    bool PasteDocument(const Ptr<SceneDocument>& doc, const Util::Guid& collection, Game::Entity parent,
        Util::Array<Game::Entity>& created);

    /// write what the instance authored back into its own prefab file, which is what Unity's Apply
    /// does.
    /// False when the instance is not valid or its file cannot be read or written
    bool ApplyInstanceToPrefab(Game::Entity entity);

    /// the value a component gets from the prefab chain, without this scene's overrides. False when
    /// the entity
    /// is its own, a file cannot be read, or the chain does not have the component
    bool GetPrefabBaseValue(Game::Entity entity, Game::ComponentId component, Util::Blob& outValue) const;
    /// the override state of a component, or an entry with an invalid component when the entity is
    /// its own
    const PrefabOverride& FindPrefabOverride(Game::Entity entity, Game::ComponentId component) const;
    /// the override state of one extension, or an entry with an invalid extension when the entity
    /// is its own
    const PrefabOverride& FindExtensionOverride(Game::Entity entity, Util::StringAtom extension) const;
    /// set the keys one extension authored on an entity, which is what an undo of a property change
    /// puts back
    void SetExtensionOverride(Game::Entity entity, Util::StringAtom extension, const Util::Array<Util::String>& fields);
    /// put a component back to what the prefab chain gives it, which also puts back a component the
    /// instance
    /// removed and takes away one it added
    bool RevertComponent(Game::Entity entity, Game::ComponentId component);
    /// true when an entity has the component
    bool HasComponent(Game::Entity entity, Game::ComponentId component) const;
    /// set the override state of one component, which is what an undo of a revert uses
    void SetPrefabOverride(Game::Entity entity, const PrefabOverride& override);
    /// drop the override state of one component
    void ClearPrefabOverride(Game::Entity entity, Game::ComponentId component);
    /// put a prefab instance back into a released slot, which is what an undo of an unpack uses
    void RestorePrefabInstance(IndexT index, const PrefabInstance& record);
    /// take an instance out of the scene or put it back. The entities keep pointing at it, so a
    /// redo of an
    /// instantiation still finds it
    void SetPrefabInstanceValid(IndexT index, bool valid);
    /// set an entity's instance, record and overrides, which is what an undo of an unpack uses
    void SetEntityPrefabLink(Game::Entity entity, IndexT prefabInstance, const Util::Guid& prefabEntity,
        const Util::Array<PrefabOverride>& overrides);
    /// add an entity to an instance as its own, with no record behind it
    void JoinInstance(Game::Entity entity, IndexT instanceIndex);
    /// take an entity off the instance it was added to
    void LeaveInstance(Game::Entity entity);
    /// decide which instance an entity the level created belongs to. An entity belongs to the
    /// instance its
    /// parent belongs to, or to the one its collection was created by
    void ResolveInstanceMembership(Game::Entity entity);
    /// mark a record of the file as one with no entity for it, which is what a delete leaves behind
    void MarkRecordRemoved(IndexT instanceIndex, const RemovedRecord& marker);
    /// unmark a record, which is what an undo of a delete does
    void ClearRecordRemoved(IndexT instanceIndex, const Util::Guid& record);
    /// create the entity for a record the instance had marked removed, from the guid and id a
    /// command kept
    bool RevertRecord(IndexT instanceIndex, const Util::Guid& record, const Util::Guid& guid, Game::Entity entity);
    /// unlink everything one instance created. The entities keep their values and become ordinary
    /// entities
    void UnpackPrefab(Game::Entity entity);
    /// drop the cached copy of a prefab file, so the next chain read sees the file as it is now
    void InvalidateDocument(const Util::String& path);
    /// re-merge every instance in this scene that reads the given file. This is what a prefab save
    /// does to the
    /// levels that use it
    void UpdatePrefabInstances(const Util::String& path);
    /// the records an entity's chain walks, in order. Empty when the entity is its own or the chain
    /// does not
    /// resolve
    void GetPrefabChain(Game::Entity entity, Util::Array<ChainRecord>& out) const;
    /// which record each field of a component gets its value from, by reflected field index. An
    /// empty entry
    /// means this scene wrote the field
    void GetFieldSources(Game::Entity entity, Game::ComponentId component, Util::Array<ChainRecord>& out) const;

    // -- Play in editor --

    /// start simulating the sim world
    void StartPlay();
    /// discard the sim world and copy the authored data again
    void StopPlay();
    /// stop and start again from the authored data
    void RestartPlay();
    /// true if the sim world is currently being simulated
    bool IsPlaying() const;

    // -- Extension data. The scene cannot read what an extension's text holds, it only hands the
    // text back to
    // the extension that wrote it --

    /// put one entity's extension data back from the text the extension has for it. What the text
    /// names is what
    /// the entity has, and an empty text is an entity with nothing on it
    bool ApplyExtension(Game::Entity entity, const Game::LevelExtension* extension, const Util::String& text);
    /// put a document's extension entries back onto an entity. An entry the registry has no
    /// extension for is
    /// kept in the document but not applied, because the scene cannot read it
    void ApplyExtensions(const Util::String& entityName, const Util::Array<DocumentExtension>& extensions, Game::Entity entity,
        Util::HashTable<Util::Guid, Game::Entity>& map);
    /// the scene's guid to entity map, which is what a text this scene wrote names its entities by
    void BuildSceneMap(Util::HashTable<Util::Guid, Game::Entity>& outMap) const;
    /// the text each registered extension writes for an entity, against the given guid map
    void CaptureExtensions(Game::Entity entity, Util::Array<DocumentExtension>& out,
        Util::HashTable<Util::Guid, Game::Entity>& map);
    /// the text for only the given keys, which is what a linked entity writes. Writing the whole
    /// value would
    /// make the instance stop following the prefab
    void CaptureExtensionKeys(Game::Entity entity, Util::StringAtom extension, const Util::Array<Util::String>& keys,
        Util::String& out);
    /// tell the extensions which guid map to read. The extensions cannot read the scene's map
    /// themselves, and
    /// the map is only valid for the call that sets it
    void SetRefContext(Util::HashTable<Util::Guid, Game::Entity>& map);
    /// the text and the keys the prefab chain gives an entity for one extension. Both are empty
    /// when the entity
    /// is its own, a file cannot be read, or the chain has nothing for the extension
    void GetChainExtension(Game::Entity entity, Util::StringAtom extension, Util::String& outText,
        Util::Array<Util::String>& outKeys);
    /// true when the scene's marker says an entity wrote one key. With no path it asks whether the
    /// extension
    /// authored anything, which is what a drawn row asks. The marker is here and the extensions
    /// cannot read it
    bool IsKeyAuthored(Game::Entity entity, Util::StringAtom extension, const Util::String& path) const;
    /// the references one extension's text names for a document entity. The ones read from the text the
    /// new text replaces go first, because a rewritten text is not the text they were read from, and a
    /// stale reference would be written into the file as a guid the text no longer names
    static void RecordDocumentRefs(const Ptr<SceneDocument>& doc, IndexT docEntity,
        Util::StringAtom extension, const Util::String& text);

private:
    /// copy one entity into the sim world, and initialize it if the instance is new
    void MirrorEntity(Game::Entity entity);
    /// copy one component value into the sim world
    void MirrorComponent(Game::Entity entity, Game::ComponentId component);
    /// change the entity references inside component data from one world to the other
    void RemapComponent(Game::ComponentId component, void* data, bool toGame);
    /// tell the features that a component was written into the sim world
    void NotifyComponentWritten(Game::Entity gameEntity, Game::ComponentId component);
    /// rebuild the sim world from the authored data
    void RebuildSimWorld();

    /// true when an entity is under another one, which is what makes a parent assignment a loop
    bool IsDescendantOf(Game::Entity entity, Game::Entity ancestor) const;

    // -- Prefab chain --

    /// one step of a prefab chain: the file it was read from, the record in it, and the entities
    /// that file
    /// created
    struct ChainHop
    {
        Ptr<SceneDocument> doc;
        IndexT docEntity = InvalidIndex;
        Util::HashTable<Util::Guid, Game::Entity> fileMap;
    };

    /// read a prefab file, using the cached copy when there is one. Null when the file cannot be
    /// read
    Ptr<SceneDocument> LoadDocumentCached(const Util::String& path) const;
    /// walk the chain from a record of a prefab file up to the plain record the chain ends at.
    /// False when a
    /// file cannot be read, a record cannot be found, or the walk comes back to a file it already
    /// read. With
    /// an instance, each hop also carries that instance's entities, which is what turns the
    /// references in the
    /// files into entities of this scene
    bool WalkChain(const Util::String& source, const Util::Guid& entityGuid, Util::Array<ChainHop>& hops,
        IndexT instanceIndex = InvalidIndex) const;
    /// check that every prefab link in a document can be read. A broken link is written to the
    /// editor log and
    /// this answers false, which is what lets load and instantiate refuse the file
    bool ValidatePrefabLinks(const Ptr<SceneDocument>& doc) const;
    /// give every record in a document its own guid before anything is made from it. The first
    /// record with a
    /// guid keeps the one the file gave, the rest take a new one. Duplicate guids are written to
    /// the editor log
    void NormalizeDocumentGuids(const Ptr<SceneDocument>& doc) const;
    /// what one record's component gives a value: the fields it carries overwrite the bytes, and
    /// its marker says
    /// whether the component is in the chain at this point
    static void MergeRecordComponent(const DocumentComponent& component, bool& present, Util::Blob& value);
    /// the components the chain gives an entity, merged from the plain record at the bottom up to
    /// this scene's
    /// record. The present flags say which components survive the adds and removals along the way
    void MergeChainComponents(const DocumentEntity& record, const Util::Array<ChainHop>& hops,
        Util::Array<Game::ComponentId>& chainCids, Util::Array<Util::Blob>& values, Util::Array<bool>& present) const;
    /// the extension texts the chain gives an entity, merged bottom up. The scene composes the
    /// texts without
    /// reading them. Each text names the guids of the file it came from, so they are put as the guids
    /// targetMap names the targets by before the merge, which is what makes the merged text name one
    /// file's records rather than several
    void MergeChainExtensions(const DocumentEntity& record, const Util::Array<ChainHop>& hops,
        Util::Dictionary<Util::StringAtom, Util::String>& outTexts,
        Util::HashTable<Util::Guid, Game::Entity>& targetMap) const;
    /// the text with its references put as the guids targetMap names the targets by, read through the map
    /// the text came from. The scene cannot read a text, so only the extension can answer for its own
    static Util::String RemapTextRefs(const Game::LevelExtension* extension, const Util::String& text,
        const Util::HashTable<Util::Guid, Game::Entity>& fromMap,
        const Util::Dictionary<Game::Entity, Util::Guid>& toGuids);
    /// the guid to entity map for one instance's file, which is what turns the references inside
    /// that file's
    /// values into entities of this scene
    void BuildFileMap(IndexT instanceIndex, const Util::String& docSource,
        Util::HashTable<Util::Guid, Game::Entity>& outMap) const;
    /// work out what an authored write to a component overrides, against what the chain gives
    void MarkPrefabOverrides(Game::Entity entity, Game::ComponentId component, const byte* value);
    /// write a value into an authored component and mirror it, without changing what the entity has
    /// overridden
    void WriteComponentValue(Game::Entity entity, Game::ComponentId component, const void* value);
    /// the index of a field by its reflected name, or InvalidIndex
    static IndexT FindFieldIndex(Game::ComponentInterface* interface, const Util::String& field);
    /// how many bytes a field spans in the component, including the padding to the next field
    static SizeT FieldSpan(Game::ComponentInterface* interface, Game::ComponentId component, IndexT field);
    /// fill an entity from a document record: the chain merged under the record's own overrides,
    /// the components
    /// added, and the entity references fixed. The hops may be empty
    void MaterializeRecord(const DocumentEntity& record, IndexT recordIndex, const Ptr<SceneDocument>& recordDoc,
        Game::Entity entity, const Util::Array<ChainHop>& hops,
        Util::HashTable<Util::Guid, Game::Entity>& fileMap);
    /// create the entity for a record the instance gained, with the guid and id the caller kept
    void CreateInstanceEntity(IndexT instanceIndex, const DocumentEntity& record, const Ptr<SceneDocument>& doc,
        IndexT recordIndex, const Util::Guid& guid, Game::Entity entity);
    /// add an entity to an instance's set, only once
    void AppendInstanceEntity(IndexT instanceIndex, Game::Entity entity);
    /// point the entity references a record carried at the entities its file was created as
    void PatchRecordEntityRefs(IndexT docEntity, Game::Entity entity, const Ptr<SceneDocument>& doc,
        const Util::HashTable<Util::Guid, Game::Entity>& fileMap);
    /// resolve the entity references a record carries inside the value the chain is building,
    /// rather than into a
    /// world row
    void PatchRecordValueRefs(const ChainHop& hop, const DocumentEntity& record,
        Util::Array<Game::ComponentId>& chainCids, Util::Array<Util::Blob>& values) const;
    /// write what one entity authored back into the record its instance's file has for it.
    /// References to
    /// entities outside the instance cannot go into the file and stay the level's. True when the
    /// record changed
    bool ApplyRecordToDocument(const Editable& editable, IndexT instanceIndex, const Ptr<SceneDocument>& doc);
    /// give the file a record for an entity the instance added. The entity's whole value is what
    /// the record gets
    bool AddRecordToDocument(Editable& editable, IndexT instanceIndex, const Ptr<SceneDocument>& doc);
    /// put a document's recorded references back into its values as record indexes, and answer with
    /// the
    /// document's guids by record index. This is what lets a document that was read be written
    /// again
    void PrepareDocumentForWrite(const Ptr<SceneDocument>& doc, Util::Array<Util::Guid>& guidByEntityIndex) const;
    /// remove the references a document recorded for one component of one record
    void DropDocumentRefs(const Ptr<SceneDocument>& doc, IndexT docEntity, Game::ComponentId component) const;

    /// the local transform an entity needs to stand at a given world position under a parent. For
    /// an entity with
    /// a parent the world components are the result of the hierarchy, so the authored values are
    /// the ones read
    /// through the parent. A parent that is invalid or cannot be divided by answers as identity
    Game::HTransform LocalTransform(Game::World* world, Game::Entity entity, Game::Entity parent,
        const Math::vec3& position, const Math::quat& orientation, const Math::vec3& scale) const;

    Util::Guid guid;
    Shell* core;
    Util::String name;
    Game::WorldHash authoringHash;
    Game::World* authoringWorld;
    /// the world the game simulates in. Owned by the game server, the scene only uses it
    Game::World* simWorld;
    Ptr<SelectionContext> selection;
    Ptr<EditCommandManager> commandManager;
    /// the scene's own view. The camera object it drives is the shared editor camera
    EditorCamera camera;
    /// editable records indexed by the entity index in the authoring world
    Util::Array<Editable> editables;
    /// collections in the order they were created
    Util::Array<Collection> collections;
    /// the collection new entities are created in
    Util::Guid activeCollection;
    /// the file this scene was read from or last saved to. Empty for a scene that has never been
    /// saved
    Util::String filePath;
    /// the version at the time the file was saved, which is what tells a save there is nothing to
    /// write
    uint64_t savedVersion;
    /// the prefab instances. A slot left by an unpacked instance stays, so the indexes the records
    /// and the
    /// saved files hold keep meaning what they meant
    Util::Array<PrefabInstance> prefabInstances;
    /// the prefab files read for the chains, by path. The chains read these instead of the disk,
    /// and saving a
    /// prefab file drops its copy
    mutable Util::Dictionary<Util::String, Ptr<SceneDocument>> documentCache;
    /// bumped on every change to the authored data
    uint64_t version;
    bool playing;
    /// true while the sim world holds this scene's entities. Only the scene being shown has them,
    /// so a change
    /// to a scene behind a tab writes the authoring world alone
    bool inSimWorld;
};

} // namespace Editor
