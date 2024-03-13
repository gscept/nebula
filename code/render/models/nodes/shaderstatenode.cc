//------------------------------------------------------------------------------
// shaderstatenode.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "shaderstatenode.h"
#include "transformnode.h"
#include "coregraphics/shaderserver.h"
#include "resources/resourceserver.h"

#include "system_shaders/objects_shared.h"

using namespace Util;
using namespace Math;

static CoreGraphics::ShaderId baseShader = CoreGraphics::InvalidShaderId;
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
Util::FixedArray<CoreGraphics::ResourceTableId>
ShaderStateNode::CreateResourceTables()
{
    if (baseShader == CoreGraphics::InvalidShaderId)
        baseShader = CoreGraphics::ShaderGet("shd:system_shaders/objects_shared.fxb"_atm);

    Util::FixedArray<CoreGraphics::ResourceTableId> ret(CoreGraphics::GetNumBufferedFrames());

    for (IndexT i = 0; i < ret.Size(); i++)
    {
        CoreGraphics::BufferId cbo = CoreGraphics::GetConstantBuffer(i);
        CoreGraphics::ResourceTableId table = CoreGraphics::ShaderCreateResourceTable(baseShader, NEBULA_DYNAMIC_OFFSET_GROUP, 256);
        CoreGraphics::ResourceTableSetConstantBuffer(table, { cbo, ObjectsShared::Table_DynamicOffset::ObjectBlock_SLOT, 0, sizeof(ObjectsShared::ObjectBlock), 0, false, true });
        CoreGraphics::ResourceTableCommitChanges(table);
        ret[i] = table;
    }

    return ret;
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
        this->materialName = reader->ReadString();
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
    for (auto table : this->resourceTables)
        CoreGraphics::DestroyResourceTable(table);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateNode::OnFinishedLoading(ModelStreamingData* streamingData)
{
    TransformNode::OnFinishedLoading(streamingData);

    // load surface immediately, however it will load textures async
    streamingData->requiredBits |= LoadBits::MaterialBit;
    Resources::CreateResource(this->materialName, this->tag, [this, streamingData](Resources::ResourceId id)
        {
            streamingData->loadedBits |= LoadBits::MaterialBit;
            this->material = id;
        });

    this->resourceTables = ShaderStateNode::CreateResourceTables();
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateNode::DrawPacket::Apply(const CoreGraphics::CmdBufferId cmdBuf, IndexT batchIndex, IndexT bufferIndex)
{
    // Apply per-draw surface parameters
    if (this->materialInstance != Materials::MaterialInstanceId::Invalid())
        MaterialInstanceApply(this->materialInstance, cmdBuf, batchIndex, bufferIndex);

    // Set per-draw resource tables
    IndexT prevOffset = 0;
    for (IndexT i = 0; i < this->numTables; i++)
    {
        CoreGraphics::CmdSetResourceTable(cmdBuf, this->tables[i], this->slots[i], CoreGraphics::GraphicsPipeline, this->numOffsets[i], this->offsets[prevOffset]);
        prevOffset = this->numOffsets[i];
    }
}

} // namespace Models
