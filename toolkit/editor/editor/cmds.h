#pragma once
//------------------------------------------------------------------------------
/**
    @file cmds.h

    contains the api for all undo/redo commands

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor.h"

//------------------------------------------------------------------------------
/**
    Commands that are undo/redoable
*/
namespace Edit
{

Editor::Entity CreateEntity();
Editor::Entity DuplicateEntity(Editor::Entity entity);
void DeleteEntity(Editor::Entity entity);
void SetSelection(Util::Array<Editor::Entity> const& selection);
void SetComponent(Editor::Entity entity, Game::ComponentId component, void* value);
void AddComponent(Editor::Entity entity, Game::ComponentId component);
void RemoveComponent(Editor::Entity entity, Game::ComponentId component);
void SetEntityName(Editor::Entity entity, Util::String const& name);
Util::Guid CreateCollection(const Util::String& name, const Util::Guid& parent = Util::Guid());
void SetCollectionName(const Util::Guid& collection, const Util::String& name);
bool SetCollectionParent(const Util::Guid& collection, const Util::Guid& parent);
void MoveEntitiesToCollection(const Util::Array<Editor::Entity>& entities, const Util::Guid& collection);
bool DeleteCollection(const Util::Guid& collection);
bool SetParent(Editor::Entity child, Editor::Entity parent);
bool ClearParent(Editor::Entity child);
bool SetWorldPosition(Editor::Entity entity, const Game::Position& position);
bool SetWorldOrientation(Editor::Entity entity, const Game::Orientation& orientation);
bool SetWorldScale(Editor::Entity entity, const Game::Scale& scale);
void PreviewWorldTransform(Editor::Entity entity, const Game::Position& position, const Game::Orientation& orientation, const Game::Scale& scale);

} // namespace Edit
