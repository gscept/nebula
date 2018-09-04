#pragma once
//------------------------------------------------------------------------------
/**
	A model resource consists of nodes, each of which inhibit some information
	read from an .n3 file. 
	
	(C) 2017 Individual contributors, see AUTHORS file
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
	NodeHasTransform = CharacterNodeType + 1, // all nodes above inherit from TransformNode
	TransformNodeType,
	ShaderStateNodeType,
	NodeHasShaderState = ShaderStateNodeType + 1, // all nodes above inherit from ShaderStateNode
	PrimtiveNodeType,
	ParticleSystemNodeType,
	CharacterSkinNodeType,

	NumNodeTypes
};

struct ModelCreateInfo
{
	Resources::ResourceName resource;
	Util::StringAtom tag;
	std::function<void(const Resources::ResourceId)> successCallback;
	std::function<void(const Resources::ResourceId)> failCallback;
	bool async;
};

/// create model (resource)
const ModelId CreateModel(const ModelCreateInfo& info);
/// discard model (resource)
void DestroyModel(const ModelId id);

/// create model instance
const ModelInstanceId CreateModelInstance(const ModelId mdl);
/// destroy model instance
void DestroyModelInstance(const ModelInstanceId id);

class StreamModelPool;
extern StreamModelPool* modelPool;
} // namespace Models