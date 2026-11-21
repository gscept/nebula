//------------------------------------------------------------------------------
//  @file deletecollectioncommand.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/commands/deletecollectioncommand.h"

namespace Editor
{

__ImplementClass(Editor::DeleteCollectionCommand, 'DECC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
*/
const char*
DeleteCollectionCommand::Name() const
{
    return "Delete collection";
}

//------------------------------------------------------------------------------
/**
    Everything the delete moves has to be read before it happens, because the collection itself is
    gone by the time the command is undone.
*/
bool
DeleteCollectionCommand::Execute()
{
    const IndexT index = this->scene->FindCollection(this->collection);
    if (index == InvalidIndex)
        return false;

    if (!this->initialized)
    {
        this->record = this->scene->GetCollections()[index];
        this->index = index;

        for (IndexT i = 0; i < this->scene->GetCollections().Size(); i++)
        {
            if (this->scene->GetCollections()[i].parent == this->collection)
                this->childCollections.Append(this->scene->GetCollections()[i].guid);
        }

        for (IndexT i = 0; i < this->scene->GetEditables().Size(); i++)
        {
            const Editable& editable = this->scene->GetEditables()[i];
            if (editable.guid.IsValid() && editable.collection == this->collection)
                this->entities.Append(editable.entity);
        }

        this->initialized = true;
    }

    return this->scene->DeleteCollection(this->collection);
}

//------------------------------------------------------------------------------
/**
    The collection goes back at the index it was taken out of, and the children and the entities go
    back to pointing at it.
*/
bool
DeleteCollectionCommand::Unexecute()
{
    if (!this->record.guid.IsValid())
        return false;

    if (!this->scene->CreateCollection(this->record, this->index).IsValid())
        return false;

    for (IndexT i = 0; i < this->childCollections.Size(); i++)
        this->scene->SetCollectionParent(this->childCollections[i], this->record.guid);

    for (IndexT i = 0; i < this->entities.Size(); i++)
        this->scene->MoveEntityToCollection(this->entities[i], this->record.guid);

    return true;
}

} // namespace Editor
