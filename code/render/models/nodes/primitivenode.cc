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
    primitiveGroupIndex(InvalidIndex),
    primitiveGroupIndexLoaded(0)
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
std::function<void()>
PrimitiveNode::GetApplyNodeFunction()
{
    return [this]()
    {
        CoreGraphics::MeshBind(this->res, this->primitiveGroupIndex);
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
        // get mesh resource
        Resources::ResourceName meshName = reader->ReadString();

        // add as pending resource in loader
        this->primitiveGroupIndex = 0;
        this->res = Resources::CreateResource(meshName, tag, [this](Resources::ResourceId id)
            {
                this->res = id;
                this->primitiveGroupIndex = this->primitiveGroupIndexLoaded;
            }, nullptr, immediate);
    }
    else if (FourCC('PGRI') == fourcc)
    {
        // primitive group index
        this->primitiveGroupIndexLoaded = reader->ReadUInt();
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
