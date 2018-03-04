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

Ids::Id8 StreamModelPool::NodeInstanceCounter = 0;
__ImplementClass(Models::StreamModelPool, 'MOLO', Resources::ResourceStreamPool);
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
	const Util::Dictionary<Util::StringAtom, ModelNodeId>& nodes = this->modelAllocator.Get<1>(id.allocId);

	// alloc a new instance of this model
	Ids::Id32 mnid = this->modelInstanceAllocator.AllocObject();
	miid.model = id.allocId;
	miid.instance = mnid;

	// get all template nodes
	Util::Array<ModelNodeInstanceId>& nodeInstances = this->modelInstanceAllocator.Get<0>(mnid);
	SizeT i;
	for (i = 0; i < nodes.Size(); i++)
	{
		// get model node and allocate new instance
		const ModelNodeId& nid = nodes.ValueAtIndex(i);
		Ids::Id32 iid = this->nodeInstanceConstructors[nid.fourcc]();

		ModelNode* node = this->nodeAccessors[nid.fourcc](nid.node);

		// setup new id
		ModelNodeInstanceId instanceId;
		instanceId.node = nid.node;
		instanceId.instance = iid;
		instanceId.fourcc = nid.fourcc;
		nodeInstances.Append(instanceId);
	}

	// ok, all allocated, now parent
	for (i = 0; i < nodeInstances.Size(); i++)
	{
		const ModelNodeInstanceId& iid = nodeInstances[i];
		Models::ModelNode::Instance* node = this->nodeInstanceAccessors[iid.fourcc](iid.instance);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
StreamModelPool::DestroyModelInstance(const ModelId id)
{
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceStreamPool::LoadStatus
StreamModelPool::LoadFromStream(const Ids::Id24 id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream)
{
	// a model is a list of resources, a bounding box, and a dictionary of nodes
	Math::bbox& boundingBox = this->Get<0>(id);
	boundingBox.set(Math::point(0), Math::vector(0));
	Util::Dictionary<Util::StringAtom, ModelNodeId>& nodes = this->Get<1>(id);
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
					const ModelNodeId& pair = nodes.ValueAtIndex(i);
					const ModelNode* node = this->nodeAccessors[pair.fourcc](pair.node);
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
				Ids::Id32 nodeId = this->nodeConstructors[mapping]();
				ModelNode* node = this->nodeAccessors[mapping](nodeId);
				node->model = id;
				node->name = name;
				if (!this->nodeStack.IsEmpty())
				{
					const ModelNodeId& pair = this->nodeStack.Peek();
					node->parent = pair.node;
					ModelNode* parent = this->nodeAccessors[pair.fourcc](pair.node);
					parent->children.Append(nodeId);
				}
				ModelNodeId nid;
				nid.fourcc = mapping;
				nid.node = nodeId;
				this->nodeStack.Push(nid);
				nodes.Add(name, nid);
			}
			else if (fourCC == FourCC('<MND'))
			{
				// end of current ModelNode
				n_assert(!this->nodeStack.IsEmpty());
				const ModelNodeId& pair = this->nodeStack.Pop();
				ModelNode* node = this->nodeAccessors[pair.fourcc](pair.node);
				node->Setup();
			}
			else
			{
				// if not opening or closing a node, assume it's a data tag
				ModelNodeId& top = this->nodeStack.Peek();
				ModelNode* node = this->nodeAccessors[top.fourcc](top.node);
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
StreamModelPool::Unload(const Ids::Id24 id)
{
	Util::Dictionary<Util::StringAtom, ModelNodeId>& nodes = this->Get<1>(id);
	IndexT i;
	for (i = 0; i < nodes.Size(); i++)
	{
		const ModelNodeId& pair = nodes.ValueAtIndex(i);
		ModelNode* node = this->nodeAccessors[pair.fourcc](pair.node);
		node->Discard();
	}
	nodes.Clear();
}

} // namespace Models