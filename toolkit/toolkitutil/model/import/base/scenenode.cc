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

//------------------------------------------------------------------------------
/**
*/
void 
SceneNode::CalculateGlobalTransforms()
{
    Math::mat4 parentTransform = Math::mat4::identity;
    if (this->base.parent != nullptr)
    {
        parentTransform = this->base.parent->base.globalTransform;
    }

    Math::mat4 transform = Math::affine(Math::vec3(1), this->base.rotation, this->base.translation);
    this->base.globalTransform = parentTransform * transform;

    for (int i = 0; i < this->base.children.Size(); i++)
    {
        this->base.children[i]->CalculateGlobalTransforms();
    }
}

} // namespace ToolkitUtil
