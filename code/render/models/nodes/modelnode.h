#pragma once
//------------------------------------------------------------------------------
/**
    Base class for all model nodes, which is the resource level instantiation
    of hierarchical model. Each node represents some level in the hierarchy,
    and the different type of node contain their specific data.

    The nodes have names for lookup, the name is setup during load. 

    A node is a part of an N-tree, meaning there are N children [0-infinity]
    for each node. The model itself keeps track of the root node, and a dictionary
    of all nodes and their corresponding names.

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/stringatom.h"
#include "io/binaryreader.h"
#include "math/bbox.h"
#include "ids/id.h"
#include "memory/arenaallocator.h"
#include "models/model.h"
#include "materials/material.h"
#include "coregraphics/primitivegroup.h"

#include "util/delegate.h"


namespace Characters
{
class CharacterContext;
}

namespace Particles
{
class ParticleContext;
}

namespace CoreGraphics
{
struct ResourceTableId;
struct CmdBufferId;
//enum ShaderPipeline;
}

namespace Models
{

struct ModelId;
class ModelLoader;
class ModelNode
{
public:

    /// constructor
    ModelNode();
    /// destructor
    virtual ~ModelNode();

    /// return constant reference to children
    const Util::Array<ModelNode*>& GetChildren() const;
    /// get type of node
    const NodeType GetType() const;
    /// get feature bits of node
    const NodeBits GetBits() const;

    /// return true if all children should create hierarchies upon calling CreateInstance
    virtual bool GetImplicitHierarchyActivation() const;

    /// return name
    const Util::StringAtom& GetName() const;
    /// get hash
    const uint32_t HashCode() const;

    /// Get function to apply node 
    virtual std::function<void(const CoreGraphics::CmdBufferId)> GetApplyFunction();
    /// Get function to fetch primitive group
    virtual std::function<const CoreGraphics::PrimitiveGroup()> GetPrimitiveGroupFunction();

    /// Unload data (don't call explicitly)
    virtual void Unload();

protected:
    friend class ModelLoader;
    friend class ModelContext;
    friend class ModelServer;
    friend class CharacterNode;
    friend class CharacterSkinNode;
    friend class Characters::CharacterContext;
    friend class Particles::ParticleContext;

    /// load data
    virtual bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate);

    /// call when model node data is finished loading (not accounting for secondary resources)
    virtual void OnFinishedLoading(ModelStreamingData* streamingData);

    /// discard node
    virtual void Discard();

    Util::StringAtom name;
    NodeType type;
    NodeBits bits;

    Models::ModelNode* parent;
    Util::Array<Models::ModelNode*> children;
    Math::bbox boundingBox;
    Util::StringAtom tag;

    IndexT uniqueId;
    static IndexT ModelNodeUniqueIdCounter;
};

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<ModelNode*>&
ModelNode::GetChildren() const
{
    return this->children;
}

//------------------------------------------------------------------------------
/**
*/
inline const NodeType 
ModelNode::GetType() const
{
    return this->type;
}

//------------------------------------------------------------------------------
/**
*/
inline const NodeBits 
ModelNode::GetBits() const
{
    return this->bits;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom& 
ModelNode::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline const uint32_t
ModelNode::HashCode() const
{
    return (uint32_t)this->uniqueId;
}

//------------------------------------------------------------------------------
/**
*/
inline std::function<void(const CoreGraphics::CmdBufferId)> 
ModelNode::GetApplyFunction()
{
    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
inline std::function<const CoreGraphics::PrimitiveGroup()> 
ModelNode::GetPrimitiveGroupFunction()
{
    return nullptr;
}

} // namespace Models
