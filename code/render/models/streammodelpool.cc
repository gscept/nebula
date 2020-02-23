//------------------------------------------------------------------------------
// streammodelpool.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "streammodelpool.h"
#include "model.h"
#include "core/refcounted.h"
#include "io/binaryreader.h"
#include "util/fourcc.h"
#include "nodes/modelnode.h"
#include "resources/resourceserver.h"
#include "coregraphics/mesh.h"

using namespace Util;
using namespace IO;
namespace Models
{

Ids::Id8 StreamModelPool::NodeMappingCounter = 0;
__ImplementClass(Models::StreamModelPool, 'SMPO', Resources::ResourceStreamPool);
//------------------------------------------------------------------------------
/**
*/
StreamModelPool::StreamModelPool()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
StreamModelPool::~StreamModelPool()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
StreamModelPool::Setup()
{
	this->placeholderResourceName = "mdl:system/placeholder.n3";
	this->failResourceName = "mdl:system/error.n3";

	IMPLEMENT_NODE_ALLOCATOR('TRFN', TransformNode, this->transformNodes, this->transformNodeInstances);
	IMPLEMENT_NODE_ALLOCATOR('SPND', PrimitiveNode, this->primitiveNodes, this->primitiveNodeInstances);
	IMPLEMENT_NODE_ALLOCATOR('SHSN', ShaderStateNode, this->shaderStateNodes, this->shaderStateNodeInstances);
	IMPLEMENT_NODE_ALLOCATOR('CHSN', CharacterSkinNode, this->characterSkinNodes, this->characterSkinNodeInstances);
	IMPLEMENT_NODE_ALLOCATOR('CHRN', CharacterNode, this->characterNodes, this->characterNodeInstances);
	IMPLEMENT_NODE_ALLOCATOR('PSND', ParticleSystemNode, this->particleSystemNodes, this->particleSystemNodeInstances);

	// never forget to run this
	ResourceStreamPool::Setup();
}

//------------------------------------------------------------------------------
/**
*/
ModelInstanceId
StreamModelPool::CreateModelInstance(const ModelId id)
{
	ModelInstanceId miid;
	const Util::Dictionary<Util::StringAtom, Models::ModelNode*>& nodes = this->modelAllocator.Get<ModelNodes>(id.resourceId);
	SizeT& instances = this->modelAllocator.Get<InstanceCount>(id.resourceId);

	// alloc a new instance of this model
	Ids::Id32 mnid = this->modelInstanceAllocator.Alloc();
	miid.model = id.resourceId;
	miid.instance = mnid;
	instances++;

	// get all template nodes
	Util::Array<Models::ModelNode::Instance*>& nodeInstances = this->modelInstanceAllocator.Get<ModelNodeInstances>(mnid);
	Util::Array<Models::NodeType>& nodeTypes = this->modelInstanceAllocator.Get<ModelNodeTypes>(mnid);
	Memory::ArenaAllocator<MODEL_INSTANCE_MEMORY_CHUNK_SIZE>& alloc = this->modelAllocator.Get<InstanceNodeAllocator>(id.resourceId);

	// allocate memory
	byte* mem = nullptr;
	if (this->modelInstanceAllocator.Get<InstanceMemory>(mnid) == nullptr)
	{
		// id is new, allocate new buffer
		mem = (byte*)alloc.Alloc(this->modelAllocator.Get<InstanceAllocSize>(id.resourceId));
		this->modelInstanceAllocator.Get<InstanceMemory>(mnid) = mem;
	}
	else
	{
		// id is old, reuse old memory but reset it to 0
		mem = this->modelInstanceAllocator.Get<InstanceMemory>(mnid);
		memset(mem, 0x0, this->modelAllocator.Get<InstanceAllocSize>(id.resourceId));
	}

	SizeT i;
	for (i = 0; i < nodes.Size(); i++)
	{
		// get model node and allocate new instance
		Models::ModelNode* node = nodes.ValueAtIndex(i);

		// root node(s)
		if (node->parent == nullptr)
			this->CreateModelInstanceRecursive(node, 0, nullptr, &mem, nodeInstances, nodeTypes);
	}

	return miid;
}

//------------------------------------------------------------------------------
/**
*/
void
StreamModelPool::DestroyModelInstance(const ModelInstanceId id)
{
	// release allocator memory, and return index to pool
	SizeT& instances = this->modelAllocator.Get<InstanceCount>(id.model);
	instances--;
	this->modelInstanceAllocator.Get<ModelNodeInstances>(id.instance).Clear();
	this->modelInstanceAllocator.Dealloc(id.instance);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Dictionary<Util::StringAtom, Models::ModelNode*>&
StreamModelPool::GetModelNodes(const ModelId id)
{
	return this->modelAllocator.Get<ModelNodes>(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Models::ModelNode::Instance*>&
StreamModelPool::GetModelNodeInstances(const ModelInstanceId id)
{
	return this->modelInstanceAllocator.Get<ModelNodeInstances>(id.instance);
}

//------------------------------------------------------------------------------
/**
*/
const Math::bbox&
StreamModelPool::GetModelBoundingBox(const ModelId id) const
{
	return this->modelAllocator.Get<ModelBoundingBox>(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
Math::bbox&
StreamModelPool::GetModelBoundingBox(const ModelId id)
{
	return this->modelAllocator.Get<ModelBoundingBox>(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
const Math::bbox&
StreamModelPool::GetModelInstanceBoundingBox(const ModelInstanceId id) const
{
	return this->modelInstanceAllocator.Get<InstanceBoundingBox>(id.instance);
}

//------------------------------------------------------------------------------
/**
*/
Math::bbox&
StreamModelPool::GetModelInstanceBoundingBox(const ModelInstanceId id)
{
	return this->modelInstanceAllocator.Get<InstanceBoundingBox>(id.instance);
}

//------------------------------------------------------------------------------
/**
*/
uint
StreamModelPool::GetModelInstanceObjectId(const ModelInstanceId id) const
{
	return this->modelInstanceAllocator.Get<ObjectId>(id.instance);
}

//------------------------------------------------------------------------------
/**
*/
uint
& StreamModelPool::GetModelInstanceObjectId(const ModelInstanceId id)
{
	return this->modelInstanceAllocator.Get<ObjectId>(id.instance);
}

//------------------------------------------------------------------------------
/**
	Create model instance breadth first
*/
void
StreamModelPool::CreateModelInstanceRecursive(Models::ModelNode* node, const IndexT childIndex, Models::ModelNode::Instance* parentInstance, byte** memory, Util::Array<Models::ModelNode::Instance*>& instances, Util::Array<Models::NodeType>& types)
{
	Models::ModelNode::Instance* inst = node->CreateInstance(memory, parentInstance);
	instances.Append(inst);
	types.Append(node->type);

	// setup child
	if (parentInstance)
		parentInstance->children[childIndex] = inst;

	// only activate nodes if they should be activated implicitly
	if (node->GetImplicitHierarchyActivation())
		inst->active = true;
	else
		inst->active = false;

	// reserve size for children
	inst->children.Resize(node->children.Size());

	// continue recursion
	IndexT i;
	for (i = 0; i < node->children.Size(); i++)
		CreateModelInstanceRecursive(node->children[i], i, inst, memory, instances, types);
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceStreamPool::LoadStatus
StreamModelPool::LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
{
	// a model is a list of resources, a bounding box, and a dictionary of nodes
	Math::bbox& boundingBox = this->Get<ModelBoundingBox>(id);
	SizeT& instanceSize = this->Get<InstanceAllocSize>(id);
	instanceSize = 0;
	SizeT hierarchicalInstanceSize = 0;
	boundingBox.set(Math::point(0), Math::vector(0));
	Memory::ArenaAllocator<MODEL_MEMORY_CHUNK_SIZE>& allocator = this->Get<ModelNodeAllocator>(id);
	Util::Dictionary<Util::StringAtom, Models::ModelNode*>& nodes = this->Get<ModelNodes>(id);
	Models::ModelNode*& root = this->Get<RootNode>(id);
	Ptr<BinaryReader> reader = BinaryReader::Create();

	// setup stack for loading nodes
	Util::Stack<Models::ModelNode*> nodeStack;
	Util::Stack<SizeT> nodeInstanceSize;

	reader->SetStream(stream);
	if (reader->Open())
	{
		// make sure it really it's actually an n3 file and check the version
		// also, we assume that the file has host-native endianess (that's
		// ensured by the asset tools)
		FourCC magic = reader->ReadUInt();
		uint version = reader->ReadUInt();
		if (magic != FourCC('NEB3'))
		{
			n_error("StreamModelLoader: '%s' is not a N3 binary file!", stream->GetURI().AsString().AsCharPtr());
			return Failed;
		}
		if (version != 1)
		{
			n_error("StreamModelLoader: '%s' has wrong version!", stream->GetURI().AsString().AsCharPtr());
			return Failed;
		}

		// start reading tags
		bool done = false;
		while ((!stream->Eof()) && (!done))
		{
			FourCC fourCC = reader->ReadUInt();
			if (fourCC == FourCC('>MDL'))
			{
				// start of Model
				FourCC classFourCC = reader->ReadUInt();
				String name = reader->ReadString();
			}
			else if (fourCC == FourCC('<MDL'))
			{
				// end of Model, if we're reloading, we shouldn't load all resources again...
				done = true;

				// update model-global bounding box
				Math::bbox box;
				box.begin_extend();
				IndexT i;
				for (i = 0; i < nodes.Size(); i++)
				{
					const ModelNode* node = nodes.ValueAtIndex(i);
					box.extend(node->boundingBox);
				}
				box.end_extend();
				boundingBox = box;
			}
			else if (fourCC == FourCC('>MND'))
			{
				// start of a ModelNode
				FourCC classFourCC = reader->ReadUInt();
				String name = reader->ReadString();
				Ids::Id8 mapping = this->nodeFourCCMapping[classFourCC];
				ModelNode* node = this->nodeConstructors[mapping](allocator);
				node->parent = nullptr;
				node->boundingBox = Math::bbox();
				node->model = id;
				node->name = name;
				node->tag = tag;
				node->nodeAllocator = &allocator;
				instanceSize += node->GetInstanceSize();
				hierarchicalInstanceSize += node->GetInstanceSize();
				if (!nodeStack.IsEmpty())
				{
					Models::ModelNode* parent = nodeStack.Peek();
					parent->children.Append(node);
					node->parent = parent;
				}
				nodeStack.Push(node);
				nodeInstanceSize.Push(hierarchicalInstanceSize);
				nodes.Add(name, node);
			}
			else if (fourCC == FourCC('<MND'))
			{
				// end of current ModelNode
				n_assert(!nodeStack.IsEmpty());
				Models::ModelNode* node = nodeStack.Pop();
				node->hierarchicalInstanceSize = hierarchicalInstanceSize;
				hierarchicalInstanceSize = 0;
				node->OnFinishedLoading();
			}
			else
			{
				// if not opening or closing a node, assume it's a data tag
				ModelNode* node = nodeStack.Peek();
				if (!node->Load(fourCC, tag, reader, immediate))
				{
					break;
				}
			}
		}
		n_assert(nodeStack.IsEmpty());
		reader->Close();
	}
	return Success;
}

//------------------------------------------------------------------------------
/**
*/
void
StreamModelPool::Unload(const Resources::ResourceId id)
{
	const SizeT& instances = this->Get<InstanceCount>(id);
	if (instances > 0)
		n_error("Model '%s' still has active instances!", this->names[id.poolId].Value());
	Util::Dictionary<Util::StringAtom, Models::ModelNode*>& nodes = this->Get<ModelNodes>(id);

	// unload nodes
	for (IndexT i = 0; i < nodes.Size(); i++)
	{
		nodes.ValueAtIndex(i)->Unload();
	}
	nodes.Clear();

	this->Get<ModelNodeAllocator>(id).Release();
	this->Get<InstanceNodeAllocator>(id).Release();
	this->Get<RootNode>(id) = nullptr;

	this->states[id.poolId] = Resources::Resource::State::Unloaded;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Dictionary<Util::StringAtom, Models::ModelNode*>&
ModelGetNodes(const ModelId id)
{
	return modelPool->GetModelNodes(id);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Models::ModelNode::Instance*>&
ModelInstanceGetNodes(const ModelInstanceId id)
{
	return modelPool->GetModelNodeInstances(id);
}

} // namespace Models
