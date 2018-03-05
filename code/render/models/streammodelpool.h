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

private:
	friend class PrimitiveNode;
	friend class CharacterNode;
	friend class CharacterSkinNode;
	friend class ParticleSystemNode;
	friend class ModelContext;

	/// create an instance of a model recursively
	void CreateModelInstanceRecursive(Models::ModelNode* parent, Models::ModelNode::Instance* parentInstance, Memory::ChunkAllocator<0xFFF>& allocator, Util::Array<Models::ModelNode::Instance*>& instances);

	/// perform actual load, override in subclass
	LoadStatus LoadFromStream(const Ids::Id24 id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream);
	/// unload resource
	void Unload(const Ids::Id24 id);

	///
	Util::Stack<Models::ModelNode*> nodeStack;
	Util::Dictionary<Util::FourCC, Ids::Id8> nodeFourCCMapping;

	Ids::IdAllocator<
		Math::bbox,													// total bounding box
		Memory::ChunkAllocator<0xFFF>,								// memory allocator
		Util::Dictionary<Util::StringAtom, Models::ModelNode*>,		// nodes
		Models::ModelNode*,											// root
		SizeT														// instances
	> modelAllocator;
	__ImplementResourceAllocator(modelAllocator);

	Util::Array<std::function<Models::ModelNode*(Memory::ChunkAllocator<0xFFF>&)>> nodeConstructors;
	Util::Array<std::function<Models::ModelNode::Instance*(Memory::ChunkAllocator<0xFFF>&)>> nodeInstanceConstructors;

	Ids::IdAllocator<
		Memory::ChunkAllocator<0xFFF>,						// memory allocator
		Util::Array<Models::ModelNode::Instance*>,			// list of node instances
		Math::matrix44,										// transform
		Math::bbox											// transformed bounding box
	> modelInstanceAllocator;

	static Ids::Id8 NodeMappingCounter;

#define IMPLEMENT_NODE_ALLOCATOR(FourCC, Type, NodeList, NodeInstanceList) \
	nodeConstructors.Append([this](Memory::ChunkAllocator<0xFFF>& alloc) -> Models::ModelNode* { return alloc.Alloc<Models::Type>(); }); \
	nodeInstanceConstructors.Append([this](Memory::ChunkAllocator<0xFFF>& alloc) -> Models::ModelNode::Instance* { return alloc.Alloc<Models::Type::Instance>(); }); \
	this->nodeFourCCMapping.Add(FourCC, NodeMappingCounter++);
};
} // namespace Models
