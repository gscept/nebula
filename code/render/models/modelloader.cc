//------------------------------------------------------------------------------
// streammodelcache.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "modelloader.h"
#include "model.h"
#include "core/refcounted.h"
#include "io/binaryreader.h"
#include "util/fourcc.h"
#include "nodes/modelnode.h"

using namespace Util;
using namespace IO;
namespace Models
{

__ImplementClass(Models::ModelLoader, 'SMPO', Resources::ResourceLoader);
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
    this->placeholderResourceName = "sysmdl:placeholder.n3";
    this->failResourceName = "sysmdl:error.n3";

    IMPLEMENT_NODE_ALLOCATOR('TRFN', TransformNode);
    IMPLEMENT_NODE_ALLOCATOR('SPND', PrimitiveNode);
    IMPLEMENT_NODE_ALLOCATOR('SHSN', ShaderStateNode);
    IMPLEMENT_NODE_ALLOCATOR('CHSN', CharacterSkinNode);
    IMPLEMENT_NODE_ALLOCATOR('CHRN', CharacterNode);
    IMPLEMENT_NODE_ALLOCATOR('PSND', ParticleSystemNode);

    // never forget to run this
    ResourceLoader::Setup();
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceUnknownId
ModelLoader::InitializeResource(Ids::Id32 entry, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
{
    // a model is a list of resources, a bounding box, and a dictionary of nodes
    Math::bbox boundingBox;
    boundingBox.set(Math::vec3(0), Math::vec3(0));
    Util::Array<Models::ModelNode*> nodes;
    Ptr<BinaryReader> reader = BinaryReader::Create();

    // setup stack for loading nodes
    Util::Stack<Models::ModelNode*> nodeStack;

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
            return Resources::InvalidResourceUnknownId;
        }
        if (version != 1)
        {
            n_error("StreamModelLoader: '%s' has wrong version!", stream->GetURI().AsString().AsCharPtr());
            return Resources::InvalidResourceUnknownId;
        }

        // start reading tags
        bool done = false;
        while ((!stream->Eof()) && (!done))
        {
            FourCC fourCC = reader->ReadUInt();
            if (fourCC == FourCC('>MDL'))
            {
                // start of Model
                UNUSED(FourCC) classFourCC = reader->ReadUInt();
                UNUSED(String) name = reader->ReadString();
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
                    const ModelNode* node = nodes[i];
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
                ModelNode* node = this->nodeConstructors[classFourCC]();
                node->parent = nullptr;
                node->boundingBox = Math::bbox();
                node->name = name;
                node->tag = tag;
                if (!nodeStack.IsEmpty())
                {
                    Models::ModelNode* parent = nodeStack.Peek();
                    parent->children.Append(node);
                    node->parent = parent;
                }
                nodeStack.Push(node);
                nodes.Append(node);
            }
            else if (fourCC == FourCC('<MND'))
            {
                // end of current ModelNode
                n_assert(!nodeStack.IsEmpty());
                Models::ModelNode* node = nodeStack.Pop();
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

    ModelCreateInfo createInfo;
    createInfo.boundingBox = boundingBox;
    createInfo.nodes = nodes;
    ModelId id = CreateModel(createInfo);
    return id;
}

//------------------------------------------------------------------------------
/**
*/
void
ModelLoader::Unload(const Resources::ResourceId id)
{
    ModelId model;
    model.resourceId = id.resourceId;
    model.resourceType = id.resourceType;
    const Util::Array<Models::ModelNode*>& nodes = ModelGetNodes(model);

    // unload nodes
    for (IndexT i = 0; i < nodes.Size(); i++)
    {
        nodes[i]->Unload();
    }

    DestroyModel(model);
}

} // namespace Models
