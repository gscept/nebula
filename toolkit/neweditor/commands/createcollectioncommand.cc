//------------------------------------------------------------------------------
//  @file createcollectioncommand.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/commands/createcollectioncommand.h"

namespace Editor
{

__ImplementClass(Editor::CreateCollectionCommand, 'CRCC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
*/
const char*
CreateCollectionCommand::Name() const
{
    return "Create collection";
}

//------------------------------------------------------------------------------
/**
    A guid that is already a collection is not an action, so the command says it did not run.
*/
bool
CreateCollectionCommand::Execute()
{
    if (!this->record.guid.IsValid())
    {
        this->record.guid.Generate();
        this->record.order = (IndexT)this->scene->GetCollections().Size();
    }

    if (this->scene->FindCollection(this->record.guid) != InvalidIndex)
        return false;

    return this->scene->CreateCollection(this->record, InvalidIndex).IsValid();
}

//------------------------------------------------------------------------------
/**
    Deleting a collection moves what is in it to the parent, which is the same thing the command did
    on the way in. By the time this runs, the moves that put things in the collection have already
    been undone.
*/
bool
CreateCollectionCommand::Unexecute()
{
    if (this->scene->FindCollection(this->record.guid) == InvalidIndex)
        return false;

    this->scene->DeleteCollection(this->record.guid);
    return true;
}

} // namespace Editor
