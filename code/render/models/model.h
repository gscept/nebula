#pragma once
//------------------------------------------------------------------------------
/**
    A model resource consists of nodes, each of which inhibit some information
    read from an .n3 file. 
    
    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idallocator.h"
#include "resources/resourceid.h"
#include "core/rttimacros.h"
#include <functional>
namespace Models
{

class ModelNode;

RESOURCE_ID_TYPE(ModelId);

#define MODEL_MEMORY_CHUNK_SIZE 0x1000

enum NodeType
{
    CharacterNodeType,
    TransformNodeType,
    ShaderStateNodeType,
    PrimitiveNodeType,
    ParticleSystemNodeType,
    CharacterSkinNodeType,

    NumNodeTypes
};

enum NodeBits
{
    NoBits = N_BIT(0),
    HasTransformBit = N_BIT(1),
    HasStateBit = N_BIT(2)
};
__ImplementEnumBitOperators(NodeBits);

struct NodeInstanceRange
{
    SizeT begin, end;
};

/// create model (resource)
const ModelId CreateModel(const ResourceCreateInfo& info);
/// discard model (resource)
void DestroyModel(const ModelId id);

class StreamModelCache;
extern StreamModelCache* modelPool;
} // namespace Models
