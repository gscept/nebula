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
#include "model.h"
#include "nodes/primitivenode.h"
#include "nodes/shaderstatenode.h"
#include "nodes/transformnode.h"
#include "nodes/characternode.h"
#include "nodes/characterskinnode.h"
#include "nodes/particlesystemnode.h"

namespace Visibility
{
class VisibilityContext;
}
namespace Models
{

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
	void DestroyModelInstance(const ModelInstanceId id);

	/// get bounding box of model
	const Math::bbox& GetModelBoundingBox(const ModelId id) const;
	/// get bounding box of model
	Math::bbox& GetModelBoundingBox(const ModelId id);
	
	/// get bounding box of model instance
	const Math::bbox& GetModelInstanceBoundingBox(const ModelInstanceId id) const;
	/// get bounding box of model which we can change
	Math::bbox& GetModelInstanceBoundingBox(const ModelInstanceId id);

private:
	friend class PrimitiveNode;
	friend class CharacterNode;
	friend class CharacterSkinNode;
	friend class ParticleSystemNode;
	friend class ModelContext;
	friend class Visibility::VisibilityContext;

	/// create an instance of a model recursively
	void CreateModelInstanceRecursive(Models::ModelNode* node, Models::ModelNode::Instance* parentInstance, byte* memory, Util::Array<Models::ModelNode::Instance*>& instances, Util::Array<Models::NodeType>& types);

	/// perform actual load, override in subclass
	LoadStatus LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream);
	/// unload resource
	void Unload(const Resources::ResourceId id);

	///
	Util::Stack<Models::ModelNode*> nodeStack;
	Util::Dictionary<Util::FourCC, Ids::Id8> nodeFourCCMapping;

	Ids::IdAllocator<
		Math::bbox,													// 0 - total bounding box
		Memory::ChunkAllocator<MODEL_MEMORY_CHUNK_SIZE>,			// 1 - memory allocator
		Util::Dictionary<Util::StringAtom, Models::ModelNode*>,		// 2 - nodes
		Models::ModelNode*,											// 3 - root
		SizeT,														// 4 - instances
		SizeT,														// 5 - instance size
		Memory::ChunkAllocator<MODEL_INSTANCE_MEMORY_CHUNK_SIZE>	// 6 - instance allocator
	> modelAllocator;
	__ImplementResourceAllocator(modelAllocator);

	Util::Array<std::function<Models::ModelNode*(Memory::ChunkAllocator<MODEL_MEMORY_CHUNK_SIZE>&)>> nodeConstructors;
	Util::Array<std::function<Models::ModelNode::Instance*(Memory::ChunkAllocator<MODEL_INSTANCE_MEMORY_CHUNK_SIZE>&)>> nodeInstanceConstructors;

	Ids::IdAllocator<
		Util::Array<Models::ModelNode::Instance*>,					// list of node instances
		Util::Array<Models::NodeType>,								// node instance types
		Math::matrix44,												// transform
		Math::bbox													// transformed bounding box
	> modelInstanceAllocator;

	static Ids::Id8 NodeMappingCounter;

#define IMPLEMENT_NODE_ALLOCATOR(FourCC, Type, NodeList, NodeInstanceList) \
	nodeConstructors.Append([this](Memory::ChunkAllocator<MODEL_MEMORY_CHUNK_SIZE>& alloc) -> Models::ModelNode* { return alloc.Alloc<Models::Type>(); }); \
	nodeInstanceConstructors.Append([this](Memory::ChunkAllocator<MODEL_INSTANCE_MEMORY_CHUNK_SIZE>& alloc) -> Models::ModelNode::Instance* { return alloc.Alloc<Models::Type::Instance>(); }); \
	this->nodeFourCCMapping.Add(FourCC, NodeMappingCounter++);
};
} // namespace Models
