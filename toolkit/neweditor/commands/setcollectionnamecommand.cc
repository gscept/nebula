//------------------------------------------------------------------------------
//  @file setcollectionnamecommand.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/commands/setcollectionnamecommand.h"

namespace Editor
{

__ImplementClass(Editor::SetCollectionNameCommand, 'SCNC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
*/
const char*
SetCollectionNameCommand::Name() const
{
    return "Set collection name";
}

//------------------------------------------------------------------------------
/**
*/
bool
SetCollectionNameCommand::Execute()
{
    const IndexT index = this->scene->FindCollection(this->collection);
    if (index == InvalidIndex)
        return false;

    if (!this->initialized)
    {
        this->oldName = this->scene->GetCollections()[index].name;
        this->initialized = true;
    }

    return this->scene->SetCollectionName(this->collection, this->newName);
}

//------------------------------------------------------------------------------
/**
*/
bool
SetCollectionNameCommand::Unexecute()
{
    return this->scene->SetCollectionName(this->collection, this->oldName);
}

} // namespace Editor
