#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::SceneNode
  
    A node in a scene node tree. 
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "util/stringatom.h"
#include "util/variant.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class SceneNode : public Core::RefCounted
{
    __DeclareClass(SceneNode);
public:
    /// a node attribute
    struct Attr
    {
        Util::StringAtom name;
        Util::FourCC fourCC;
        Util::Array<Util::Variant> values;
    };

    /// constructor
    SceneNode();
    /// destructor
    virtual ~SceneNode();
    
    /// setup the node
    void Setup(const Util::StringAtom& name, const Util::StringAtom& type, Ptr<SceneNode> parent);
    /// discard the node
    void Discard();
    /// return true if the node has been setup
    bool IsValid() const;

    /// get name of the node
    const Util::StringAtom& GetName() const;
    /// set type (post-setup)
    void SetType(const Util::StringAtom& t);
    /// get type of the node
    const Util::StringAtom& GetType() const;
    
    /// get the number of children of the node
    SizeT GetNumChildren() const;
    /// get child at index
    const Ptr<SceneNode>& GetChildByIndex(IndexT i) const;
    /// check if a child exists by name
    bool HasChild(const Util::StringAtom& name) const;
    /// lookup child by name, return invalid ptr if child not found
    Ptr<SceneNode> GetChildByName(const Util::StringAtom& name) const;
    /// check if node has a parent
    bool HasParent() const;
    /// get pointer to parent, return invalid ptr if has no parent
    const Ptr<SceneNode>& GetParent() const;

    /// add an attribute to the node
    void AddAttr(const Util::StringAtom& name, const Util::FourCC& fourCC, const Util::Array<Util::Variant>& values);
    /// get number of attributes on the node, optionally of specific type
    SizeT GetNumAttrs(const Util::StringAtom& filter = "") const;
    /// get attribute at index
    const Attr& GetAttr(IndexT index, const Util::StringAtom& filter = "") const;
    /// get all attributes of node
    const Util::Array<Attr>& GetAllAttrs() const;
 
private:
    /// add a child node (this is called from the Setup method of the child!)
    void AddChild(Ptr<SceneNode> child);

    Util::StringAtom name;
    Util::StringAtom type;
    Ptr<SceneNode> parent;
    Util::Array<Ptr<SceneNode>> children;
    Util::Dictionary<Util::StringAtom,IndexT> childIndexMap;
    Util::Array<Attr> attrs;
    bool isValid;
};

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<SceneNode::Attr>&
SceneNode::GetAllAttrs() const
{
    return this->attrs;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
