#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::NglTFNode

    Encapsulates an gltf node as a Nebula-friendly object

    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "math/quat.h"
#include "math/vec4.h"
#include "animutil/animbuilderclip.h"
#include "coreanimation/curvetype.h"
#include "coreanimation/infinitytype.h"
#include "animutil/animbuilder.h"
#include "modelutil/modelattributes.h"
#include "gltf/gltfdata.h"
#include "core/weakptr.h"

#define KEYS_PER_MS 40

//------------------------------------------------------------------------------
namespace ToolkitUtil
{

class NglTFScene;
class NglTFMesh;
class NglTFNode : public Core::RefCounted
{
    __DeclareClass(NglTFNode);
public:
    /// constructor
    NglTFNode();
    /// destructor
    virtual ~NglTFNode();

    /// sets up node
    virtual void Setup(Gltf::Node const* node, const Ptr<NglTFScene>& scene);
    /// discards the node
    virtual void Discard();

    /// extracts rotation, translation and scaling
    virtual void ExtractTransform();
    /// extracts rotation, translation and scaling
    virtual void ExtractTransform(const Math::mat4& transform);

    /// sets the initial rotation (overrides extracted data)
    void SetInitialRotation(const Math::quat& rot);
    /// returns the initial rotation
    const Math::quat& GetInitialRotation() const;
    /// sets the initial position (overrides extracted data)
    void SetInitialPosition(const Math::vec4& pos);
    /// returns the initial position
    const Math::vec4& GetInitialPosition() const;
    /// sets the initial scale (overrides extracted data)
    void SetInitialScale(const Math::vec4& scale);
    /// returns the initial scale
    const Math::vec4& GetInitialScale() const;
    /// returns transform
    const Math::mat4& GetTransform() const;

    /// adds a child to this node
    void AddChild(const Ptr<NglTFNode>& child);
    /// removes a child to this node
    void RemoveChild(const Ptr<NglTFNode>& child);
    /// returns a child to this node
    const Ptr<NglTFNode>& GetChild(IndexT index) const;
    /// returns the number of children
    const IndexT GetChildCount() const;
    /// return index of child
    const IndexT IndexOfChild(const Ptr<NglTFNode>& child);
    /// set the parent
    void SetParent(const Ptr<NglTFNode>& parent);
    /// returns pointer to parent
    const Ptr<NglTFNode>& GetParent() const;

    /// returns true if node is a part of the physics hierarchy
    const bool IsPhysics() const;

    /// returns FBX node pointer
    Gltf::Node const* GetNode() const;

    /// sets the node name
    void SetName(const Util::String& name);
    /// gets the node name
    const Util::String& GetName() const;

protected:
    friend class NglTFScene;

    const Gltf::Document*           gltfScene;
    Gltf::Node const*               gltfNode;
    Util::Array<Ptr<NglTFNode> >    children;
    Ptr<NglTFNode>                  parent;
    WeakPtr<NglTFScene>             scene;

    Util::String                    name;

    Math::quat                  rotation;
    Math::vec4                  position;
    Math::vec4                  scale;
    Math::mat4                  transform;
};

//------------------------------------------------------------------------------
/**
*/
inline void
NglTFNode::SetInitialRotation(const Math::quat& rot)
{
    this->rotation = rot;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::quat&
NglTFNode::GetInitialRotation() const
{
    return this->rotation;
}

//------------------------------------------------------------------------------
/**
*/
inline void
NglTFNode::SetInitialPosition(const Math::vec4& pos)
{
    this->position = pos;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::vec4&
NglTFNode::GetInitialPosition() const
{
    return this->position;
}

//------------------------------------------------------------------------------
/**
*/
inline void
NglTFNode::SetInitialScale(const Math::vec4& scale)
{
    this->scale = scale;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::vec4&
    NglTFNode::GetInitialScale() const
{
    return this->scale;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::mat4&
NglTFNode::GetTransform() const
{
    return this->transform;
}

//------------------------------------------------------------------------------
/**
*/
inline Gltf::Node const*
NglTFNode::GetNode() const
{
    n_assert(this->gltfNode);
    return this->gltfNode;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<NglTFNode>&
NglTFNode::GetParent() const
{
    return this->parent;
}

//------------------------------------------------------------------------------
/**
*/
inline void
NglTFNode::SetParent(const Ptr<NglTFNode>& parent)
{
    this->parent = parent;
}

//------------------------------------------------------------------------------
/**
*/
inline const IndexT
NglTFNode::IndexOfChild(const Ptr<NglTFNode>& child)
{
    IndexT index = this->children.FindIndex(child);
    n_assert(index != InvalidIndex);
    return index;
}

//------------------------------------------------------------------------------
/**
*/
inline void
NglTFNode::SetName(const Util::String& name)
{
    n_assert(name.IsValid());
    this->name = name;
}


//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
NglTFNode::GetName() const
{
    return this->name;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------