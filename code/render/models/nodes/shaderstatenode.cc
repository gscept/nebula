//------------------------------------------------------------------------------
// shaderstatenode.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "shaderstatenode.h"
#include "transformnode.h"
#include "coregraphics/shaderserver.h"
#include "resources/resourceserver.h"

#include "objects_shared.h"

using namespace Util;
using namespace Math;
namespace Models
{
//------------------------------------------------------------------------------
/**
*/
ShaderStateNode::ShaderStateNode()
{
    this->type = ShaderStateNodeType;
    this->bits = HasTransformBit | HasStateBit;
}

//------------------------------------------------------------------------------
/**
*/
ShaderStateNode::~ShaderStateNode()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void 
ShaderStateNode::SetMaxLOD(const float lod)
{
    Materials::materialCache->SetMaxLOD(this->materialRes, lod);
}

//------------------------------------------------------------------------------
/**
*/
bool
ShaderStateNode::Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate)
{
    bool retval = true;
    if (FourCC('MNMT') == fourcc)
    {
        // this isn't used, it's been deprecated, the 'MATE' tag is the relevant one
        Util::String materialName = reader->ReadString();
    }
    else if (FourCC('MATE') == fourcc)
    {
        this->materialName = reader->ReadString() + NEBULA_SURFACE_EXTENSION;
    }
    else if (FourCC('STXT') == fourcc)
    {
        // ShaderTexture
        StringAtom paramName = reader->ReadString();
        StringAtom paramValue = reader->ReadString();
        String fullTexResId = String(paramValue.AsString() + NEBULA_TEXTURE_EXTENSION);
    }
    else if (FourCC('SINT') == fourcc)
    {
        // ShaderInt
        StringAtom paramName = reader->ReadString();
        int paramValue = reader->ReadInt();
    }
    else if (FourCC('SFLT') == fourcc)
    {
        // ShaderFloat
        StringAtom paramName = reader->ReadString();
        float paramValue = reader->ReadFloat();
    }
    else if (FourCC('SBOO') == fourcc)
    {
        // ShaderBool
        StringAtom paramName = reader->ReadString();
        bool paramValue = reader->ReadBool();
    }
    else if (FourCC('SFV2') == fourcc)
    {
        // ShaderVector
        StringAtom paramName = reader->ReadString();
        vec2 paramValue = reader->ReadFloat2();
    }
    else if (FourCC('SFV4') == fourcc)
    {
        // ShaderVector
        StringAtom paramName = reader->ReadString();
        vec4 paramValue = reader->ReadVec4();
    }
    else if (FourCC('STUS') == fourcc)
    {
        // @todo: implement universal indexed shader parameters!
        // shaderparameter used by multilayered nodes
        int index = reader->ReadInt();
        vec4 paramValue = reader->ReadVec4();
        String paramName("MLPUVStretch");
        paramName.AppendInt(index);
    }
    else if (FourCC('SSPI') == fourcc)
    {
        // @todo: implement universal indexed shader parameters!
        // shaderparameter used by multilayered nodes
        int index = reader->ReadInt();
        vec4 paramValue = reader->ReadVec4();
        String paramName("MLPSpecIntensity");
        paramName.AppendInt(index);
    }
    else
    {
        retval = TransformNode::Load(fourcc, tag, reader, immediate);
    }
    return retval;
}

//------------------------------------------------------------------------------
/**
*/
void 
ShaderStateNode::Unload()
{
    // free material
    Resources::DiscardResource(this->materialRes);

    // destroy table and constant buffer
    CoreGraphics::DestroyResourceTable(this->resourceTable);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateNode::OnFinishedLoading()
{
    TransformNode::OnFinishedLoading();

    // load surface immediately, however it will load textures async
    this->materialRes = Resources::CreateResource(this->materialName, this->tag, nullptr, nullptr, true);
    this->shaderConfig = Materials::materialCache->GetType(this->materialRes);
    this->material = Materials::materialCache->GetId(this->materialRes);
    CoreGraphics::ShaderId shader = CoreGraphics::ShaderServer::Instance()->GetShader("shd:objects_shared.fxb"_atm);
    CoreGraphics::BufferId cbo = CoreGraphics::GetGraphicsConstantBuffer();
    this->objectTransformsIndex = ObjectsShared::Table_DynamicOffset::ObjectBlock::SLOT;
    this->instancingTransformsIndex = ObjectsShared::Table_DynamicOffset::InstancingBlock::SLOT;
    this->skinningTransformsIndex = ObjectsShared::Table_DynamicOffset::JointBlock::SLOT;

    this->resourceTable = CoreGraphics::ShaderCreateResourceTable(shader, NEBULA_DYNAMIC_OFFSET_GROUP, 256);
    CoreGraphics::ResourceTableSetConstantBuffer(this->resourceTable, { cbo, this->objectTransformsIndex, 0, sizeof(ObjectsShared::ObjectBlock), 0, false, true });
    CoreGraphics::ResourceTableCommitChanges(this->resourceTable);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateNode::DrawPacket::Apply(const CoreGraphics::CmdBufferId cmdBuf, IndexT batchIndex, Materials::ShaderConfig* type)
{
    // Apply per-draw surface parameters
    if (this->materialInstance != Materials::MaterialInstanceId::Invalid())
        type->ApplyMaterialInstance(cmdBuf, batchIndex, this->materialInstance);

    // Set per-draw resource tables
    IndexT prevOffset = 0;
    for (IndexT i = 0; i < this->numTables; i++)
    {
        CoreGraphics::CmdSetResourceTable(cmdBuf, this->tables[i], this->slots[i], CoreGraphics::GraphicsPipeline, this->numOffsets[i], this->offsets[prevOffset]);
        prevOffset = this->numOffsets[i];
    }
}

} // namespace Models
