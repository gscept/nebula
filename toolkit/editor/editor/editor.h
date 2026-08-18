#pragma once
//------------------------------------------------------------------------------
/**
    @class  Editor::Editor

    Front end for the Nebula editor

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/string.h"
#include "util/guid.h"
#include "game/category.h"
#include "game/gameserver.h"

namespace Editor
{

constexpr uint32_t WORLD_EDITOR = uint32_t('EWLD');
constexpr uint32_t TIMESOURCE_EDITOR = uint32_t('TsEd');

typedef Game::Entity Entity;

struct Editable
{
    /// guid
    Util::Guid guid;
    /// name
    Util::String name;
    /// which game entity in the game database the editable is associated with
    Game::Entity gameEntity = Game::Entity::Invalid();
    /// version of editable. Bump if something has changed about the entity.
    uint64_t version = 0;
    /// editor collection containing this entity, invalid means scene root
    Util::Guid collection;
    /// order within the collection
    uint collectionOrder = 0;

};

struct Collection
{
    /// persistent collection identity
    Util::Guid guid;
    /// display name
    Util::String name;
    /// parent collection, invalid means scene root
    Util::Guid parent;
    /// order among sibling collections
    uint order = 0;
};

struct State
{
    /// contains the world state for the editor
    Game::World* editorWorld;
    /// maps from editor entity index to editable
    Util::Array<Editable> editables;
    /// editor-only level collections
    Util::Array<Collection> collections;
    /// current collection used for newly created entities
    Util::Guid activeCollection;
    /// path to the currently loaded level source
    Util::String levelPath;
};

/// Create the editor
void Create();

/// Start the editor
void Start();

/// Destroy the editor
void Destroy();

/// Start playing the game.
void PlayGame();

/// Pause the game.
void PauseGame();

/// Stop the game
void StopGame();

/// Load a JSON level into the editor. Instantiated levels get fresh GUIDs and do not become the active level.
bool LoadLevel(const Util::String& path, bool instantiate = false);
/// Save the active JSON level.
bool SaveLevel();
/// Save the JSON level to a new path and make it active.
bool SaveLevelAs(const Util::String& path);
/// Save the active level, opening a dialog when there is no active path or when requested.
bool SaveLevelWithDialog(bool saveAs = false);
/// Find a collection by persistent GUID.
IndexT FindCollection(const Util::Guid& guid);
/// Convert an editor-world entity reference to its mirrored game entity.
Game::Entity ToGameEntity(Game::Entity entity);
/// Convert a mirrored game entity reference to its editor-world entity.
Game::Entity ToEditorEntity(Game::Entity entity);
/// Remap all direct entity fields in component data to the game world.
void RemapComponentToGame(Game::ComponentId component, void* data);
/// Remap all direct entity fields in component data to the editor world.
void RemapComponentToEditor(Game::ComponentId component, void* data);
/// Remap all direct entity fields on an allocated entity instance to the game world.
void RemapInstanceToGame(Game::World* world, Game::Entity entity);

/// global editor state
extern State state;

} // namespace Editor
