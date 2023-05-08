//------------------------------------------------------------------------------
// primitivenode.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "primitivenode.h"
#include "resources/resourceserver.h"
#include "coregraphics/mesh.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/meshresource.h"

using namespace Util;
namespace Models
{


//------------------------------------------------------------------------------
/**
*/
PrimitiveNode::PrimitiveNode() :
    primitiveGroupIndex(InvalidIndex)
{
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
        Resources::ResourceName meshName = reader->ReadString();

        // Load directly, since the model is already loaded on a thread, this is fine
        //this->primitiveGroupIndex = 0;
        this->res = Resources::CreateResource(meshName, tag, nullptr, nullptr, true);

    }
    else if (FourCC('MSHI') == fourcc)
    {
        CoreGraphics::MeshResourceId meshRes = this->res;
        IndexT meshIndex = reader->ReadUInt();
        this->mesh = MeshResourceGetMesh(meshRes, meshIndex);

        this->vbo = CoreGraphics::MeshGetVertexBuffer(this->mesh, 0);
        this->ibo = CoreGraphics::MeshGetIndexBuffer(this->mesh);
        this->topology = CoreGraphics::MeshGetTopology(this->mesh);
        this->baseVboOffset = CoreGraphics::MeshGetVertexOffset(this->mesh, 0);
        this->attributesVboOffset = CoreGraphics::MeshGetVertexOffset(this->mesh, 1);
        this->iboOffset = CoreGraphics::MeshGetIndexOffset(this->mesh);
        this->indexType = CoreGraphics::MeshGetIndexType(this->mesh);
        this->vertexLayout = CoreGraphics::MeshGetVertexLayout(this->mesh);
    }
    else if (FourCC('PGRI') == fourcc)
    {
        // primitive group index
        this->primitiveGroupIndex = reader->ReadUInt();
        this->primGroup = CoreGraphics::MeshGetPrimitiveGroups(this->mesh)[this->primitiveGroupIndex];
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

} // namespace Models
