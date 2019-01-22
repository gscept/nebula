#pragma once
//------------------------------------------------------------------------------
/**
	A model resource consists of nodes, each of which inhibit some information
	read from an .n3 file. 
	
	(C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idallocator.h"
#include "resources/resourceid.h"
#include <functional>
namespace Models
{

RESOURCE_ID_TYPE(ModelId);
ID_32_32_NAMED_TYPE(ModelInstanceId, model, instance);

#define MODEL_MEMORY_CHUNK_SIZE 0x800
#define MODEL_INSTANCE_MEMORY_CHUNK_SIZE 0x1000

enum NodeType
{
	CharacterNodeType,
	TransformNodeType,
	NodeHasTransform = TransformNodeType, // all nodes above and equals has a transform
	ShaderStateNodeType,
	NodeHasShaderState = ShaderStateNodeType, // all nodes above and equals has a shader state
	PrimitiveNodeType,
	ParticleSystemNodeType,
	CharacterSkinNodeType,

	NumNodeTypes
};

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