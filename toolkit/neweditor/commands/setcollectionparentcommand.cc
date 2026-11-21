//------------------------------------------------------------------------------
//  @file setcollectionparentcommand.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/commands/setcollectionparentcommand.h"

namespace Editor
{

__ImplementClass(Editor::SetCollectionParentCommand, 'SCPC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
*/
const char*
SetCollectionParentCommand::Name() const
{
    return "Set collection parent";
}

//------------------------------------------------------------------------------
/**
    A collection cannot be nested inside itself, which is the check the scene does and the reason
    this can fail.
*/
bool
SetCollectionParentCommand::Execute()
{
    const IndexT index = this->scene->FindCollection(this->collection);
    if (index == InvalidIndex)
        return false;

    if (!this->initialized)
    {
        this->oldParent = this->scene->GetCollections()[index].parent;
        this->initialized = true;
    }

    return this->scene->SetCollectionParent(this->collection, this->newParent);
}

//------------------------------------------------------------------------------
/**
*/
bool
SetCollectionParentCommand::Unexecute()
{
    return this->scene->SetCollectionParent(this->collection, this->oldParent);
}

} // namespace Editor
