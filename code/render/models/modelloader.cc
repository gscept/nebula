//------------------------------------------------------------------------------
// modelloader.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "modelloader.h"
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

__ImplementClass(Models::ModelLoader, 'MOLO', Resources::ResourceLoader);
//------------------------------------------------------------------------------
/**
*/
ModelLoader::ModelLoader()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ModelLoader::~ModelLoader()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
ModelLoader::Setup()
{
	this->resourceClass = &Models::Model::RTTI;
	this->placeholderResourceId = "mdl:system/placeholder.n3";
	this->errorResourceId = "mdl:system/error.n3";
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceLoader::LoadStatus
ModelLoader::Load(const Ptr<Resources::Resource>& res, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream)
{
	Ptr<Model> model = res;
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
				n_assert(model->IsInstanceOf(classFourCC));
			}
			else if (fourCC == FourCC('<MDL'))
			{
				// end of Model, if we're reloading, we shouldn't load all resources again...
				done = true;

				// load all resources
				IndexT i;
				for (i = 0; i < this->pendingResources.Size(); i++)
				{
					// load resources using same tag
					Resources::ResourceId id = Resources::CreateResource(pendingResources[i], tag);
					model->resources.Append(id);
				}
				this->pendingResources.Clear();

				// update model-global bounding box
				Math::bbox boundingBox;
				boundingBox.begin_extend();
				for (i = 0; i < model->nodes.Size(); i++)
				{
					const Ptr<ModelNode>& node = model->nodes[i];
					boundingBox.extend(node->boundingBox);
				}
				boundingBox.end_extend();
				model->boundingBox = boundingBox;
			}
			else if (fourCC == FourCC('>MND'))
			{
				// start of a ModelNode
				FourCC classFourCC = reader->ReadUInt();
				String name = reader->ReadString();
				Ptr<ModelNode> node = (ModelNode*)Core::Factory::Instance()->Create(classFourCC);
				n_assert(node->IsA(ModelNode::RTTI));
				node->name = name;
				if (!this->nodeStack.IsEmpty())
				{
					node->parent = this->nodeStack.Peek();
					node->parent->children.Append(node);
				}
				this->nodeStack.Push(node);
				model->nodes.Add(name, node);
			}
			else if (fourCC == FourCC('<MND'))
			{
				// end of current ModelNode
				n_assert(!this->nodeStack.IsEmpty());
				this->nodeStack.Pop();
			}
			else
			{
				// if not opening or closing a node, assume it's a data tag
				if (!this->nodeStack.Peek()->Load(fourCC, this, reader))
				{
					break;
				}
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
ModelLoader::Unload(const Ptr<Resources::Resource>& res)
{
	Ptr<Model> model = res;
	IndexT i;
	for (i = 0; i < model->resources.Size(); i++)
	{
		// discard resources
		Resources::DiscardResource(model->resources[i]);
	}
	model->resources.Clear();
}

} // namespace Models