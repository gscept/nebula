//------------------------------------------------------------------------------
// primitivenode.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "primitivenode.h"
#include "resources/resourceserver.h"
#include "coregraphics/mesh.h"
#include "coregraphics/graphicsdevice.h"

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
std::function<void(const CoreGraphics::CmdBufferId)>
PrimitiveNode::GetApplyFunction()
{
    return [this](const CoreGraphics::CmdBufferId id)
    {
        // setup pipeline (a bit ugly)
        CoreGraphics::CmdSetPrimitiveTopology(id, this->topology);
        CoreGraphics::CmdSetVertexLayout(id, this->vertexLayout);

        // bind vertex buffers
        CoreGraphics::CmdSetVertexBuffer(id, 0, this->vbo, this->vboOffset);

        if (this->ibo != CoreGraphics::InvalidBufferId)
            CoreGraphics::CmdSetIndexBuffer(id, this->ibo, this->iboOffset);
    };
}

//------------------------------------------------------------------------------
/**
*/
std::function<const CoreGraphics::PrimitiveGroup()>
PrimitiveNode::GetPrimitiveGroupFunction()
{
    return [this]()
    {
        return CoreGraphics::MeshGetPrimitiveGroups(this->res)[this->primitiveGroupIndex];
    };
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
        this->primitiveGroupIndex = 0;
        this->res = Resources::CreateResource(meshName, tag, nullptr, nullptr, true);

        this->vbo = CoreGraphics::MeshGetVertexBuffer(this->res, 0);
        this->ibo = CoreGraphics::MeshGetIndexBuffer(this->res);
        this->topology = CoreGraphics::MeshGetTopology(this->res);
        this->primGroup = CoreGraphics::MeshGetPrimitiveGroups(this->res)[this->primitiveGroupIndex];
        this->vertexLayout = this->primGroup.GetVertexLayout();
        this->vboOffset = CoreGraphics::MeshGetVertexOffset(this->res, 0);
        this->iboOffset = 0;
    }
    else if (FourCC('PGRI') == fourcc)
    {
        // primitive group index
        this->primitiveGroupIndex = reader->ReadUInt();
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
