//------------------------------------------------------------------------------
// streammodelpool.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "streammodelpool.h"
#include "model.h"
#include "core/refcounted.h"
#include "io/binaryreader.h"
#include "util/fourcc.h"
#include "nodes/modelnode.h"
#include "resources/resourcemanager.h"
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
	this->placeholderResourceId = "mdl:system/placeholder.n3";
	this->errorResourceId = "mdl:system/error.n3";

	IMPLEMENT_NODE_ALLOCATOR('TRFN', TransformNode, this->transformNodes, this->transformNodeInstances);
	IMPLEMENT_NODE_ALLOCATOR('SPND', PrimitiveNode, this->primitiveNodes, this->primitiveNodeInstances);
	IMPLEMENT_NODE_ALLOCATOR('SHSN', ShaderStateNode, this->shaderStateNodes, this->shaderStateNodeInstances);
	IMPLEMENT_NODE_ALLOCATOR('CHSN', CharacterSkinNode, this->characterSkinNodes, this->characterSkinNodeInstances);
	IMPLEMENT_NODE_ALLOCATOR('CHRN', CharacterNode, this->characterNodes, this->characterNodeInstances);
	IMPLEMENT_NODE_ALLOCATOR('PSND', ParticleSystemNode, this->particleSystemNodes, this->particleSystemNodeInstances);
}

//------------------------------------------------------------------------------
/**
*/
ModelInstanceId
StreamModelPool::CreateModelInstance(const ModelId id)
{
	ModelInstanceId miid;
	const Util::Dictionary<Util::StringAtom, Models::ModelNode*>& nodes = this->modelAllocator.Get<2>(id.allocId);
	SizeT& instances = this->modelAllocator.Get<4>(id.allocId);

	// alloc a new instance of this model
	Ids::Id32 mnid = this->modelInstanceAllocator.AllocObject();
	miid.model = id.allocId;
	miid.instance = mnid;
	instances++;

	// get all template nodes
	Util::Array<Models::ModelNode::Instance*>& nodeInstances = this->modelInstanceAllocator.Get<1>(mnid);
	Memory::ChunkAllocator<0xFFF>& alloc = this->modelInstanceAllocator.Get<0>(mnid);
	SizeT i;
	for (i = 0; i < nodes.Size(); i++)
	{
		// get model node and allocate new instance
		Models::ModelNode* node = nodes.ValueAtIndex(i);

		// root node(s)
		if (node->parent == nullptr)
			this->CreateModelInstanceRecursive(node, nullptr, alloc, nodeInstances);
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
	SizeT& instances = this->modelAllocator.Get<4>(id.model);
	instances--;
	this->modelInstanceAllocator.Get<0>(id.instance).Release();
	this->modelInstanceAllocator.Get<1>(id.instance).Clear();
	this->modelInstanceAllocator.DeallocObject(id.instance);
}

//------------------------------------------------------------------------------
/**
*/
const Math::bbox&
StreamModelPool::GetModelBoundingBox(const ModelId id) const
{
	return this->modelAllocator.Get<0>(id.allocId);
}

//------------------------------------------------------------------------------
/**
*/
Math::bbox&
StreamModelPool::GetModelBoundingBox(const ModelId id)
{
	return this->modelAllocator.Get<0>(id.allocId);
} 

//------------------------------------------------------------------------------
/**
*/
const Math::bbox&
StreamModelPool::GetModelBoundingBox(const ModelInstanceId id)
{
	return this->modelInstanceAllocator.Get<3>(id.model);
}

//------------------------------------------------------------------------------
/**
*/
const Math::bbox&
StreamModelPool::GetModelInstanceBoundingBox(const ModelInstanceId id)
{
	return this->modelInstanceAllocator.Get<3>(id.instance);
}

//------------------------------------------------------------------------------
/**
	Create model instance breadth first
*/
void
StreamModelPool::CreateModelInstanceRecursive(Models::ModelNode* parent, Models::ModelNode::Instance* parentInstance, Memory::ChunkAllocator<0xFFF>& allocator, Util::Array<Models::ModelNode::Instance*>& instances)
{
	IndexT offset = instances.Size();
	SizeT i;
	for (i = 0; i < parent->children.Size(); i++)
	{
		Models::ModelNode::Instance* inst = parent->children[i]->CreateInstance(allocator);
		inst->parent = parentInstance;
		inst->node = parent;
		instances.Append(inst);
	}

	// continue recursion
	for (i = 0; i < parent->children.Size(); i++)
		CreateModelInstanceRecursive(parent->children[i], instances[offset + i], allocator, instances);
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceStreamPool::LoadStatus
StreamModelPool::LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream)
{
	// a model is a list of resources, a bounding box, and a dictionary of nodes
	Math::bbox& boundingBox = this->Get<0>(id);
	boundingBox.set(Math::point(0), Math::vector(0));
	Memory::ChunkAllocator<0xFFF>& allocator = this->Get<1>(id);
	Util::Dictionary<Util::StringAtom, Models::ModelNode*>& nodes = this->Get<2>(id);
	Models::ModelNode*& root = this->Get<3>(id);
	Ptr<BinaryReader> reader = BinaryReader::Create();
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
				Math::bbox boundingBox;
				boundingBox.begin_extend();
				IndexT i;
				for (i = 0; i < nodes.Size(); i++)
				{
					const ModelNode* node = nodes.ValueAtIndex(i);
					boundingBox.extend(node->boundingBox);
				}
				boundingBox.end_extend();
				boundingBox = boundingBox;
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
				if (!this->nodeStack.IsEmpty())
				{
					Models::ModelNode* parent = this->nodeStack.Peek();
					parent->children.Append(node);
					node->parent = parent;
				}
				this->nodeStack.Push(node);
				nodes.Add(name, node);
			}
			else if (fourCC == FourCC('<MND'))
			{
				// end of current ModelNode
				n_assert(!this->nodeStack.IsEmpty());
				Models::ModelNode* node = this->nodeStack.Pop();
				node->Setup();
			}
			else
			{
				// if not opening or closing a node, assume it's a data tag
				ModelNode* node = this->nodeStack.Peek();
				if (!node->Load(fourCC, tag, reader))
				{
					break;
				}

				// call callback after we are done loading
				node->OnFinishedLoading();
			}
		}
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
	const SizeT& instances = this->Get<4>(id);
	if (instances > 0)
		n_error("Model '%s' still has active instances!", this->names[id.poolId].Value());
	Util::Dictionary<Util::StringAtom, Models::ModelNode*>& nodes = this->Get<2>(id);
	nodes.Clear();
	this->Get<1>(id).Release();
}

} // namespace Models