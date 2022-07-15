//------------------------------------------------------------------------------
// streammodelcache.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "streammodelcache.h"
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

Ids::Id8 StreamModelCache::NodeMappingCounter = 0;
__ImplementClass(Models::StreamModelCache, 'SMPO', Resources::ResourceStreamCache);
//------------------------------------------------------------------------------
/**
*/
StreamModelCache::StreamModelCache()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
StreamModelCache::~StreamModelCache()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
StreamModelCache::Setup()
{
    this->placeholderResourceName = "mdl:system/placeholder.n3";
    this->failResourceName = "mdl:system/error.n3";

    IMPLEMENT_NODE_ALLOCATOR('TRFN', TransformNode, this->transformNodes);
    IMPLEMENT_NODE_ALLOCATOR('SPND', PrimitiveNode, this->primitiveNodes);
    IMPLEMENT_NODE_ALLOCATOR('SHSN', ShaderStateNode, this->shaderStateNodes);
    IMPLEMENT_NODE_ALLOCATOR('CHSN', CharacterSkinNode, this->characterSkinNodes);
    IMPLEMENT_NODE_ALLOCATOR('CHRN', CharacterNode, this->characterNodes);
    IMPLEMENT_NODE_ALLOCATOR('PSND', ParticleSystemNode, this->particleSystemNodes);

    // never forget to run this
    ResourceStreamCache::Setup();
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Models::ModelNode*>&
StreamModelCache::GetModelNodes(const ModelId id)
{
    return this->modelAllocator.Get<ModelNodes>(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
const Math::bbox&
StreamModelCache::GetModelBoundingBox(const ModelId id) const
{
    return this->modelAllocator.Get<ModelBoundingBox>(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
Math::bbox&
StreamModelCache::GetModelBoundingBox(const ModelId id)
{
    return this->modelAllocator.Get<ModelBoundingBox>(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceStreamCache::LoadStatus
StreamModelCache::LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
{
    // a model is a list of resources, a bounding box, and a dictionary of nodes
    Math::bbox& boundingBox = this->Get<ModelBoundingBox>(id);
    boundingBox.set(Math::vec3(0), Math::vec3(0));
    Memory::ArenaAllocator<MODEL_MEMORY_CHUNK_SIZE>& allocator = this->Get<ModelNodeAllocator>(id);
    Util::Array<Models::ModelNode*>& nodes = this->Get<ModelNodes>(id);
    Models::ModelNode*& root = this->Get<RootNode>(id);
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
                Ids::Id8 mapping = this->nodeFourCCMapping[classFourCC];
                ModelNode* node = this->nodeConstructors[mapping](allocator);
                node->parent = nullptr;
                node->boundingBox = Math::bbox();
                node->model = id;
                node->name = name;
                node->tag = tag;
                node->nodeAllocator = &allocator;
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
    return Success;
}

//------------------------------------------------------------------------------
/**
*/
void
StreamModelCache::Unload(const Resources::ResourceId id)
{
    Util::Array<Models::ModelNode*>& nodes = this->Get<ModelNodes>(id);

    // unload nodes
    for (IndexT i = 0; i < nodes.Size(); i++)
    {
        nodes[i]->Unload();
    }
    nodes.Clear();

    this->Get<ModelNodeAllocator>(id).Release();
    this->Get<RootNode>(id) = nullptr;

    this->states[id.poolId] = Resources::Resource::State::Unloaded;
}

} // namespace Models
