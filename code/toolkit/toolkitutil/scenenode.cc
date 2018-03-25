//------------------------------------------------------------------------------
//  scenenode.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "scenenode.h"

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::SceneNode, 'SCND', Core::RefCounted);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
SceneNode::SceneNode() :
    isValid(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
SceneNode::~SceneNode()
{
    n_assert(!this->IsValid());
}

//------------------------------------------------------------------------------
/**
*/
void
SceneNode::Setup(const StringAtom& name_, const StringAtom& type_, Ptr<SceneNode> parent_)
{
    n_assert(!this->IsValid());
    this->isValid = true;
    this->name = name_;
    this->type = type_;
    this->parent = parent_;
    if (this->parent.isvalid())
    {
        this->parent->AddChild(this);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SceneNode::Discard()
{
    n_assert(this->IsValid());
    this->parent = nullptr;
    this->children.Clear();
    this->childIndexMap.Clear();
    this->attrs.Clear();
    this->isValid = false;
}

//------------------------------------------------------------------------------
/**
*/
bool
SceneNode::IsValid() const
{
    return this->isValid;
}

//------------------------------------------------------------------------------
/**
*/
void
SceneNode::AddChild(Ptr<SceneNode> newChild)
{
    n_assert(newChild.isvalid());
    n_assert(!this->HasChild(newChild->GetName()))
    this->children.Append(newChild);
    this->childIndexMap.Add(newChild->GetName(), this->children.Size() - 1);
}

//------------------------------------------------------------------------------
/**
*/
const StringAtom&
SceneNode::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
void
SceneNode::SetType(const StringAtom& t)
{
    this->type = t;
}

//------------------------------------------------------------------------------
/**
*/
const StringAtom&
SceneNode::GetType() const
{
    return this->type;
}

//------------------------------------------------------------------------------
/**
*/
SizeT
SceneNode::GetNumChildren() const
{
    return this->children.Size();
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<SceneNode>&
SceneNode::GetChildByIndex(IndexT i) const
{
    return this->children[i];
}

//------------------------------------------------------------------------------
/**
*/
bool
SceneNode::HasChild(const StringAtom& n) const
{
    return this->childIndexMap.Contains(n);
}

//------------------------------------------------------------------------------
/**
*/
Ptr<SceneNode>
SceneNode::GetChildByName(const StringAtom& n) const
{
    Ptr<SceneNode> result;
    if (this->childIndexMap.Contains(n))
    {
        result = this->children[this->childIndexMap[n]];
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
bool
SceneNode::HasParent() const
{
    return this->parent.isvalid();
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<SceneNode>&
SceneNode::GetParent() const
{
    return this->parent;
}

//------------------------------------------------------------------------------
/**
*/
void
SceneNode::AddAttr(const StringAtom& attrName, const FourCC& attrFourCC, const Array<Variant>& values)
{
    Attr newAttr;
    newAttr.name   = attrName;
    newAttr.fourCC = attrFourCC;
    newAttr.values = values;
    this->attrs.Append(newAttr);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
SceneNode::GetNumAttrs(const StringAtom& filter) const
{
    if (filter.IsValid())
    {
        SizeT result = 0;
        IndexT i;
        for (i = 0; i < this->attrs.Size(); i++)
        {
            if (this->attrs[i].name == filter)
            {
                result++;
            }
        }
        return result;
    }
    else
    {
        return this->attrs.Size();
    }
}

//------------------------------------------------------------------------------
/**
*/
const SceneNode::Attr&
SceneNode::GetAttr(IndexT index, const StringAtom& filter) const
{
    if (filter.IsValid())
    {
        IndexT sparseIndex = 0;
        IndexT i;
        for (i = 0; i < this->attrs.Size(); i++)
        {
            if (this->attrs[i].name == filter)
            {
                if (index == sparseIndex)
                {
                    return this->attrs[i];
                }
                sparseIndex++;
            }
        }
        n_error("SceneNode::GetAttr(): no attribute '%s' on node (name=%s, type=%s)!\n",
            filter.Value(), this->name.Value(), this->type.Value());
        return this->attrs[0];
    }
    else
    {
        return this->attrs[index];
    }
}

} // namespace ToolkitUtil