//------------------------------------------------------------------------------
//  ngltfnode.h
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "ngltfnode.h"
#include "ngltfscene.h"
#include "modelutil/clip.h"
#include "modelutil/clipevent.h"
#include "modelutil/take.h"

namespace ToolkitUtil
{

using namespace Math;
using namespace CoreAnimation;
using namespace Util;
using namespace ToolkitUtil;

__ImplementClass(ToolkitUtil::NglTFNode, 'ASNO', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
NglTFNode::NglTFNode() :
    parent(nullptr),
    position(0,0,0,1),
    rotation(),
    scale(0,0,0,0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
NglTFNode::~NglTFNode()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
NglTFNode::Setup(Gltf::Node const* node, const Ptr<NglTFScene>& scene)
{
    n_assert(node);
    this->gltfNode = node;
    this->gltfScene = scene->GetScene();
    this->scene = scene;
    this->name = node->name;
    this->ExtractTransform();
}

//------------------------------------------------------------------------------
/**
*/
void
NglTFNode::Discard()
{
    this->gltfNode = 0;
    this->parent = nullptr;
    this->children.Clear();
}

//------------------------------------------------------------------------------
/**
    Adds a child to this node, the childs parent has to be 0
*/
void
NglTFNode::AddChild(const Ptr<NglTFNode>& child)
{
    n_assert(child);
    n_assert(!child->parent.isvalid());
    this->children.Append(child);
    child->SetParent(this);
}

//------------------------------------------------------------------------------
/**
    Removes child to this node, also uncouples parent
*/
void
NglTFNode::RemoveChild(const Ptr<NglTFNode>& child)
{
    n_assert(child);
    n_assert(this == child->parent);
    this->children.EraseIndex(this->children.FindIndex(child));
    child->parent = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<NglTFNode>&
NglTFNode::GetChild(IndexT index) const
{
    n_assert(index >= 0 && index < this->children.Size());
    return this->children[index];
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
NglTFNode::GetChildCount() const
{
    return this->children.Size();
}

//------------------------------------------------------------------------------
/**
    Extract node generic transform data
*/
void
NglTFNode::ExtractTransform()
{
    Math::mat4 localTrans = this->gltfNode->matrix;
    this->transform = this->gltfNode->matrix;
    
    if (this->gltfNode->hasTRS)
    {
        this->position = this->gltfNode->translation.vec;
        this->rotation = this->gltfNode->rotation.vec;
        this->scale = this->gltfNode->scale.vec;
    }
    else
    {
        this->ExtractTransform(localTrans);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
NglTFNode::ExtractTransform(const Math::mat4& localTrans)
{
    Math::quat rotation;
    Math::vec3 translation;
    Math::vec3 scale;
    double sign;

    // decompose elements
    decompose(localTrans, scale, rotation, translation);
    sign = determinant(localTrans);

    // construct nebula matrix
    this->transform = localTrans;

    float factor = 1.0f;
    float scaleFactor = 1.0f;

    // TODO: global scene scale
    //bool result = this->gltfScene->mMetaData && this->gltfScene->mMetaData->Get("UnitScaleFactor", factor);
    //if (result)
    //{
    //  // calculate inverse scale
    //  scaleFactor = NglTFScene::Instance()->GetScale() * (1 / factor);
    //}
    //else
    {
        scaleFactor = NglTFScene::Instance()->GetScale();
    }
    
    this->rotation = rotation;
    this->position = vec4(translation[0] * scaleFactor, translation[1] * scaleFactor, translation[2] * scaleFactor, 1.0f);
    this->scale = vec4(scale[0], scale[1], scale[2], 1);
}

} // namespace ToolkitUtil