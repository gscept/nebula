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
    
    (C)2017-2020 Individual contributors, see AUTHORS file
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

#include "util/delegate.h"


#define ModelNodeInstanceCreator(type) \
inline ModelNode::Instance* type::CreateInstance(byte** memory, const ModelNode::Instance* parent)\
{\
    auto node = new (*memory) type::Instance();\
    node->Setup(this, parent);\
    *memory += sizeof(type::Instance);\
    return node;\
}

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
//enum ShaderPipeline;
}

namespace Models
{

struct ModelId;
class StreamModelPool;
class ModelNode
{
public:

    struct DrawPacket;
    struct Instance
    {
        const ModelNode::Instance* parent;          // pointer to parent
        ModelNode* node;                            // pointer to resource-level node
        Util::FixedArray<Instance*> children;       // children
        bool active : 1;

        /// setup new instance
        virtual void Setup(Models::ModelNode* node, const Models::ModelNode::Instance* parent);

        /// return size of draw packet for allocation
        virtual SizeT GetDrawPacketSize() const;
        /// fill draw packet
        virtual Models::ModelNode::DrawPacket* UpdateDrawPacket(void* mem);

        /// update prior to drawing
        virtual void Update();
    };

    struct DrawPacket
    {
        Models::ModelNode::Instance* node = nullptr;
        Materials::SurfaceInstanceId* surfaceInstance;
        SizeT* numTables = nullptr;
        CoreGraphics::ResourceTableId* tables = nullptr;
        uint32* numOffsets = nullptr;
        uint32* offsets = nullptr;
        IndexT* slots = nullptr;

        /// apply the resource tables and offsets
        void Apply(Materials::MaterialType* type);
    };

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
    /// create an instance of a node, override in the leaf classes
    virtual ModelNode::Instance* CreateInstance(byte** memory, const Models::ModelNode::Instance* parent);

    /// get size of instance
    virtual const SizeT GetInstanceSize() const { return sizeof(Instance); }
    /// return true if all children should create hierarchies upon calling CreateInstance
    virtual bool GetImplicitHierarchyActivation() const;

    /// return name
    const Util::StringAtom& GetName() const;
    /// get hash
    const uint32_t HashCode() const;

    /// apply node-level state
    virtual void ApplyNodeState();
    /// apply node-level resources
    virtual void ApplyNodeResources();

protected:
    friend class StreamModelPool;
    friend class ModelContext;
    friend class ModelServer;
    friend class CharacterNode;
    friend class CharacterSkinNode;
    friend class Characters::CharacterContext;
    friend class Particles::ParticleContext;

    /// load data
    virtual bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate);
    /// unload data
    virtual void Unload();
    /// call when model node data is finished loading (not accounting for secondary resources)
    virtual void OnFinishedLoading();

    /// discard node
    virtual void Discard();

    Util::StringAtom name;
    NodeType type;
    NodeBits bits;

    Memory::ArenaAllocator<MODEL_MEMORY_CHUNK_SIZE>* nodeAllocator;
    Models::ModelNode* parent;
    ModelId model;
    Util::Array<Models::ModelNode*> children;
    Math::bbox boundingBox;
    Util::StringAtom tag;
    SizeT hierarchicalInstanceSize;

    IndexT uniqueId;
    static IndexT ModelNodeUniqueIdCounter;
};

//------------------------------------------------------------------------------
/**
*/
inline ModelNode::Instance*
ModelNode::CreateInstance(byte** memory, const Models::ModelNode::Instance* parent)
{
    return nullptr;
}

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

} // namespace Models
