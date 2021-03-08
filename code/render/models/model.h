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
ID_32_32_NAMED_TYPE(ModelInstanceId, model, instance);

#define MODEL_MEMORY_CHUNK_SIZE 0x1000
#define MODEL_INSTANCE_MEMORY_CHUNK_SIZE 0x4000

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

/// create model (resource)
const ModelId CreateModel(const ResourceCreateInfo& info);
/// discard model (resource)
void DestroyModel(const ModelId id);

/// create model instance
const ModelInstanceId CreateModelInstance(const ModelId mdl);
/// destroy model instance
void DestroyModelInstance(const ModelInstanceId id);


class StreamModelPool;
extern StreamModelPool* modelPool;
} // namespace Models
