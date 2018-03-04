#pragma once
//------------------------------------------------------------------------------
/**
	Implements a resource loader for models
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourcestreampool.h"
#include "util/stack.h"
#include "nodes/primitivenode.h"
#include "nodes/shaderstatenode.h"
#include "nodes/transformnode.h"
#include "nodes/characternode.h"
#include "nodes/characterskinnode.h"
#include "nodes/particlesystemnode.h"

namespace Models
{

enum NodeType
{
	TransformNodeType,
	PrimtiveNodeType,
	ShaderStateNodeType,
	CharacterSkinNodeType,
	CharacterNodeType,
	ParticleSystemNodeType,

	NumNodeTypes
};

RESOURCE_ID_TYPE(ModelId);
ID_32_32_NAMED_TYPE(ModelInstanceId, model, instance);
ID_24_8_NAMED_TYPE(ModelNodeId, fourcc, node);
ID_32_24_8_NAMED_TYPE(ModelNodeInstanceId, node, instance, fourcc);

class ModelNode;
class StreamModelPool : public Resources::ResourceStreamPool
{
	__DeclareClass(StreamModelPool);
public:
	/// constructor
	StreamModelPool();
	/// destructor
	virtual ~StreamModelPool();

	/// setup resource loader, initiates the placeholder and error resources if valid
	void Setup();

	/// create an instance of a model
	ModelInstanceId CreateModelInstance(const ModelId id);
	/// destroy an instance of a model
	void DestroyModelInstance(const ModelId id);

private:
	friend class PrimitiveNode;
	friend class CharacterNode;
	friend class CharacterSkinNode;
	friend class ParticleSystemNode;

	/// perform actual load, override in subclass
	LoadStatus LoadFromStream(const Ids::Id24 id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream);
	/// unload resource
	void Unload(const Ids::Id24 id);

	Util::Stack<ModelNodeId> nodeStack;

	Util::Array<std::function<Ids::Id32()>> nodeConstructors;
	Util::Array<std::function<ModelNode*(Ids::Id32)>> nodeAccessors;
	Util::Array<TransformNode> transformNodes;
	Util::Array<PrimitiveNode> primitiveNodes;
	Util::Array<ShaderStateNode> shaderStateNodes;
	Util::Array<CharacterSkinNode> characterSkinNodes;
	Util::Array<CharacterNode> characterNodes;
	Util::Array<ParticleSystemNode> particleSystemNodes;

	Util::Dictionary<Util::FourCC, Ids::Id8> nodeFourCCMapping;

	Ids::IdAllocator<
		Math::bbox,
		Util::Dictionary<Util::StringAtom, ModelNodeId>
	> modelAllocator;
	__ImplementResourceAllocator(modelAllocator);

	Util::Array<std::function<Ids::Id32()>> nodeInstanceConstructors;
	Util::Array<std::function<ModelNode::Instance*(Ids::Id32)>> nodeInstanceAccessors;
	Util::Array<TransformNode::Instance> transformNodeInstances;
	Util::Array<PrimitiveNode::Instance> primitiveNodeInstances;
	Util::Array<ShaderStateNode::Instance> shaderStateNodeInstances;
	Util::Array<CharacterSkinNode::Instance> characterSkinNodeInstances;
	Util::Array<CharacterNode::Instance> characterNodeInstances;
	Util::Array<ParticleSystemNode::Instance> particleSystemNodeInstances;

	Ids::IdAllocator<
		Util::Array<ModelNodeInstanceId>
	> modelInstanceAllocator;

	static Ids::Id8 NodeInstanceCounter;

#define IMPLEMENT_NODE_ALLOCATOR(FourCC, NodeType, NodeList, NodeInstanceList) \
	nodeConstructors.Append([this]() -> Ids::Id32 { NodeList.Append(NodeType()); return NodeList.Size() - 1; }); \
	nodeAccessors.Append([this](Ids::Id32 id) -> Models::ModelNode* { return &NodeList[id]; }); \
	nodeInstanceConstructors.Append([this]() -> Ids::Id32 { NodeInstanceList.Append(NodeType::Instance()); return NodeInstanceList.Size() - 1; }); \
	nodeInstanceAccessors.Append([this](Ids::Id32 id) -> Models::ModelNode::Instance* { return &NodeInstanceList[id]; }); \
	this->nodeFourCCMapping.Add(FourCC, NodeInstanceCounter++);
};
} // namespace Models
