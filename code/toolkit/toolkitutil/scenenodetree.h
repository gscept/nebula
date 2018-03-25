#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::SceneNodeTree
    
    Containter for a hierarchy of SceneNodes.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"
#include "toolkitutil/scenenode.h"
#include "toolkitutil/logger.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class SceneNodeTree : public Core::RefCounted
{
    __DeclareClass(SceneNodeTree);
public:
    /// constructor
    SceneNodeTree();
    /// destructor
    virtual ~SceneNodeTree();
    
    /// setup the object
    void Setup();
    /// discard the object
    void Discard();
    /// return true if object has been setup
    bool IsValid() const;
    /// dump scene node structure to logger
    void Dump(Logger& logger);
    
    /// add a node to the scene
    void AddNode(const Ptr<SceneNode>& node);
    /// get number of nodes in the scene
    SizeT GetNumNodes() const;
    /// get node by index
    const Ptr<SceneNode>& GetNodeByIndex(IndexT i) const;

    /// return true if node exists 
    bool HasNode(const Util::String& path) const;
    /// lookup a node, returns invalid ptr if not found
    Ptr<SceneNode> LookupNode(const Util::String& path) const;
    /// get ptr to root node
    Ptr<SceneNode> GetRootNode() const;

private:    
    Util::Array<Ptr<SceneNode> > nodes;
    bool isValid;
};

} // namespace Maya
//------------------------------------------------------------------------------
    