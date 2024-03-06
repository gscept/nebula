//------------------------------------------------------------------------------
// primitivenode.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "primitivenode.h"
#include "resources/resourceserver.h"
#include "coregraphics/mesh.h"
#include "coregraphics/meshresource.h"

using namespace Util;
namespace Models
{


//------------------------------------------------------------------------------
/**
*/
PrimitiveNode::PrimitiveNode()
{
    this->loadContext.meshIndex = 0;
    this->loadContext.primIndex = 0;
    this->type = PrimitiveNodeType;
    this->bits = HasTransformBit | HasStateBit;
}

//------------------------------------------------------------------------------
/**
*/
PrimitiveNode::~PrimitiveNode()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
PrimitiveNode::Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate)
{         
    bool retval = true;
    if (FourCC('MESH') == fourcc)
    {
        // Get mesh resource
        this->meshResource = reader->ReadString();
    }
    else if (FourCC('MSHI') == fourcc)
    {
        uint index = reader->ReadUInt();
        this->loadContext.meshIndex = index;
    }
    else if (FourCC('PGRI') == fourcc)
    {
        // primitive group index
        uint index = reader->ReadUInt();
        this->loadContext.primIndex = index;
    }
    else
    {
        retval = ShaderStateNode::Load(fourcc, tag, reader, immediate);
    }
    return retval;
}

//------------------------------------------------------------------------------
/**
*/
void 
PrimitiveNode::Unload()
{
    Resources::DiscardResource(this->res);
}

//------------------------------------------------------------------------------
/**
*/
void
PrimitiveNode::OnFinishedLoading(ModelStreamingData* streamingData)
{
    ShaderStateNode::OnFinishedLoading(streamingData);
    streamingData->requiredBits |= LoadBits::MeshBit;
    Resources::CreateResource(
        this->meshResource,
        tag,
        [this, streamingData](Resources::ResourceId id)
        {
            this->res = id;
            CoreGraphics::MeshResourceId meshRes = this->res;
            this->mesh = MeshResourceGetMesh(meshRes, this->loadContext.meshIndex);
            this->vbo = CoreGraphics::MeshGetVertexBuffer(this->mesh, 0);
            this->ibo = CoreGraphics::MeshGetIndexBuffer(this->mesh);
            this->topology = CoreGraphics::MeshGetTopology(this->mesh);
            this->baseVboOffset = CoreGraphics::MeshGetVertexOffset(this->mesh, 0);
            this->attributesVboOffset = CoreGraphics::MeshGetVertexOffset(this->mesh, 1);
            this->iboOffset = CoreGraphics::MeshGetIndexOffset(this->mesh);
            this->indexType = CoreGraphics::MeshGetIndexType(this->mesh);
            this->vertexLayout = CoreGraphics::MeshGetVertexLayout(this->mesh);
            this->primGroup = CoreGraphics::MeshGetPrimitiveGroups(this->mesh)[this->loadContext.primIndex];
            streamingData->loadedBits |= LoadBits::MeshBit;
        },
        nullptr
    );
}

} // namespace Models
