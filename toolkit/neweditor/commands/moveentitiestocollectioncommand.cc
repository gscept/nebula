//------------------------------------------------------------------------------
//  @file moveentitiestocollectioncommand.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/commands/moveentitiestocollectioncommand.h"

namespace Editor
{

__ImplementClass(Editor::MoveEntitiesToCollectionCommand, 'MECC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
*/
const char*
MoveEntitiesToCollectionCommand::Name() const
{
    return "Move entities to collection";
}

//------------------------------------------------------------------------------
/**
    Moving entities to the collection they are already in is not an action, so a drag that moves
    nothing does not become an entry.
*/
bool
MoveEntitiesToCollectionCommand::Execute()
{
    if (this->entities.IsEmpty())
        return false;

    if (!this->initialized)
    {
        for (IndexT i = 0; i < this->entities.Size(); i++)
            this->oldCollections.Append(this->scene->GetEditable(this->entities[i]).collection);
        this->initialized = true;
    }

    for (IndexT i = 0; i < this->entities.Size(); i++)
        this->scene->MoveEntityToCollection(this->entities[i], this->newCollection);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
MoveEntitiesToCollectionCommand::Unexecute()
{
    for (IndexT i = 0; i < this->entities.Size(); i++)
        this->scene->MoveEntityToCollection(this->entities[i], this->oldCollections[i]);
    return true;
}

} // namespace Editor
