//------------------------------------------------------------------------------
//  @file scenenode.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "scenenode.h"
namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
*/
void 
SceneNode::Setup(const NodeType type)
{
    this->type = type;
}

} // namespace ToolkitUtil
