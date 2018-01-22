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

	// add table for nodes, add new nodes here if required
	this->constructors.Add('TRFN', [this]() -> Ids::Id32 { this->transformNodes.Append(TransformNode()); return this->transformNodes.Size() - 1; });
	this->accessors.Add('TRFN', [this](Ids::Id32 id) -> Models::ModelNode* { return &this->transformNodes[id]; });

	this->constructors.Add('SPND', [this]() -> Ids::Id32 { this->primitiveNodes.Append(PrimitiveNode()); return this->primitiveNodes.Size() - 1; });
	this->accessors.Add('SPND', [this](Ids::Id32 id) -> Models::ModelNode* { return &this->primitiveNodes[id]; });

	this->constructors.Add('SHSN', [this]() -> Ids::Id32 { this->shaderStatenodes.Append(ShaderStateNode()); return this->shaderStatenodes.Size() - 1; });
	this->accessors.Add('SHSN', [this](Ids::Id32 id) -> Models::ModelNode* { return &this->shaderStatenodes[id]; });

	this->constructors.Add('CHSN', [this]() -> Ids::Id32 { this->characterSkinNodes.Append(CharacterSkinNode()); return this->characterSkinNodes.Size() - 1; });
	this->accessors.Add('CHSN', [this](Ids::Id32 id) -> Models::ModelNode* { return &this->characterSkinNodes[id]; });

	this->constructors.Add('CHRN', [this]() -> Ids::Id32 { this->characterNodes.Append(CharacterNode()); return this->characterNodes.Size() - 1; });
	this->accessors.Add('CHRN', [this](Ids::Id32 id) -> Models::ModelNode* { return &this->characterNodes[id]; });

	this->constructors.Add('PSND', [this]() -> Ids::Id32 { this->particleSystemNode.Append(ParticleSystemNode()); return this->particleSystemNode.Size() - 1; });
	this->accessors.Add('PSND', [this](Ids::Id32 id) -> Models::ModelNode* { return &this->particleSystemNode[id]; });

	// setup type-to-index table
	this->nodeTypeIndices[TransformNodeType] = this->accessors.FindIndex('TRFN');
	this->nodeTypeIndices[PrimtiveNodeType] = this->accessors.FindIndex('SPND');
	this->nodeTypeIndices[ShaderStateNodeType] = this->accessors.FindIndex('SHSN');
	this->nodeTypeIndices[CharacterSkinNodeType] = this->accessors.FindIndex('CHSN');
	this->nodeTypeIndices[CharacterNodeType] = this->accessors.FindIndex('CHRN');
	this->nodeTypeIndices[ParticleSystemNodeType] = this->accessors.FindIndex('PSND');
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
	Util::Dictionary<Util::StringAtom, Util::KeyValuePair<Util::FourCC, Ids::Id32>>& nodes = this->Get<1>(id);
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
					const NodeTypeId& pair = nodes.ValueAtIndex(i);
					const ModelNode* node = this->accessors[pair.Key()](pair.Value());
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
				Ids::Id32 nodeId = this->constructors[classFourCC]();
				ModelNode* node = this->accessors[classFourCC](nodeId);
				node->model = id;
				node->name = name;
				if (!this->nodeStack.IsEmpty())
				{
					const NodeTypeId& pair = this->nodeStack.Peek();
					node->parent = pair.Value();
					ModelNode* parent = this->accessors[pair.Key()](pair.Value());
					parent->children.Append(nodeId);
				}
				NodeTypeId id(classFourCC, nodeId);
				this->nodeStack.Push(id);
				nodes.Add(name, id);
			}
			else if (fourCC == FourCC('<MND'))
			{
				// end of current ModelNode
				n_assert(!this->nodeStack.IsEmpty());
				const NodeTypeId& pair = this->nodeStack.Pop();
				ModelNode* node = this->accessors[pair.Key()](pair.Value());
				node->Setup();
			}
			else
			{
				// if not opening or closing a node, assume it's a data tag
				NodeTypeId& top = this->nodeStack.Peek();
				ModelNode* node = this->accessors[top.Key()](top.Value());
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
	Util::Dictionary<Util::StringAtom, NodeTypeId>& nodes = this->Get<1>(id);
	IndexT i;
	for (i = 0; i < nodes.Size(); i++)
	{
		const NodeTypeId& pair = nodes.ValueAtIndex(i);
		ModelNode* node = this->accessors[pair.Key()](pair.Value());
		node->Discard();
	}
	nodes.Clear();
}

} // namespace Models