//------------------------------------------------------------------------------
// globalconstants.cc
// (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "globalconstants.h"
#include "coregraphics/shader.h"

#include "frame/default.h"

namespace Graphics
{

struct
{
    Util::FixedArray<CoreGraphics::ResourceTableId> frameResourceTables;
    Util::FixedArray<CoreGraphics::ResourceTableId> tickResourceTables;
    CoreGraphics::ResourcePipelineId tableLayout;

    Shared::PerTickParams::STRUCT tickParams;

    union DirtySet
    {
        struct
        {
            bool graphicsDirty : 1;
            bool computeDirty : 1;
        };
        int bits;
    };
    DirtySet tickParamsDirty;
    uint64_t tickCboOffset, viewCboOffset, shadowViewCboOffset;

    Shared::ViewConstants::STRUCT viewConstants;
    Shared::ShadowViewConstants::STRUCT shadowViewConstants;
    DirtySet viewConstantsDirty, shadowViewConstantsDirty;
} globalConstantState;

//------------------------------------------------------------------------------
/**
*/
void
CreateGlobalConstants(const GlobalConstantsCreateInfo& info)
{
    // create shader state for textures, and fetch variables
    CoreGraphics::ShaderId shader = CoreGraphics::ShaderGet("shd:system_shaders/shared.gplb"_atm);

    globalConstantState.tableLayout = CoreGraphics::ShaderGetResourcePipeline(shader);

    globalConstantState.frameResourceTables.Resize(CoreGraphics::GetNumBufferedFrames());
    globalConstantState.tickResourceTables.Resize(CoreGraphics::GetNumBufferedFrames());
    IndexT i;
    for (i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
    {
        globalConstantState.frameResourceTables[i] = CoreGraphics::ShaderCreateResourceTable(shader, NEBULA_FRAME_GROUP, globalConstantState.frameResourceTables.Size());
        CoreGraphics::ObjectSetName(globalConstantState.frameResourceTables[i], "Main Frame Group Descriptor");

        globalConstantState.tickResourceTables[i] = CoreGraphics::ShaderCreateResourceTable(shader, NEBULA_TICK_GROUP, globalConstantState.tickResourceTables.Size());
        CoreGraphics::ObjectSetName(globalConstantState.tickResourceTables[i], "Main Tick Group Descriptor");
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyGlobalConstants()
{
    for (IndexT i = 0; i < globalConstantState.frameResourceTables.Size(); i++)
    {
        CoreGraphics::DestroyResourceTable(globalConstantState.frameResourceTables[i]);
        CoreGraphics::DestroyResourceTable(globalConstantState.tickResourceTables[i]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AllocateGlobalConstants()
{
    // Allocate memory for the per-tick, per-view and shadow view matrices
    globalConstantState.tickCboOffset = CoreGraphics::AllocateConstantBufferMemory(sizeof(Shared::PerTickParams::STRUCT));
    globalConstantState.viewCboOffset = CoreGraphics::AllocateConstantBufferMemory(sizeof(Shared::ViewConstants::STRUCT));
    globalConstantState.shadowViewCboOffset = CoreGraphics::AllocateConstantBufferMemory(sizeof(Shared::ShadowViewConstants::STRUCT));
    IndexT bufferedFrameIndex = CoreGraphics::GetBufferedFrameIndex();

    // Bind tables with memory allocated
    ResourceTableSetConstantBuffer(globalConstantState.frameResourceTables[bufferedFrameIndex], { CoreGraphics::GetConstantBuffer(bufferedFrameIndex), Shared::ViewConstants::BINDING, 0, sizeof(Shared::ViewConstants::STRUCT), globalConstantState.viewCboOffset });
    ResourceTableSetConstantBuffer(globalConstantState.frameResourceTables[bufferedFrameIndex], { CoreGraphics::GetConstantBuffer(bufferedFrameIndex), Shared::ShadowViewConstants::BINDING, 0, sizeof(Shared::ShadowViewConstants::STRUCT), globalConstantState.shadowViewCboOffset });
    ResourceTableCommitChanges(globalConstantState.frameResourceTables[bufferedFrameIndex]);

    // Update tick resource tables
    ResourceTableSetConstantBuffer(globalConstantState.tickResourceTables[bufferedFrameIndex], { CoreGraphics::GetConstantBuffer(bufferedFrameIndex), Shared::PerTickParams::BINDING, 0, sizeof(Shared::PerTickParams::STRUCT), globalConstantState.tickCboOffset });
    ResourceTableCommitChanges(globalConstantState.tickResourceTables[bufferedFrameIndex]);
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateTickParams(const Shared::PerTickParams::STRUCT& tickParams)
{
    globalConstantState.tickParams = tickParams;
    globalConstantState.tickParamsDirty.bits = 0x3;
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateViewConstants(const Shared::ViewConstants::STRUCT& viewConstants)
{
    globalConstantState.viewConstants = viewConstants;
    globalConstantState.viewConstantsDirty.bits = 0x3;
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateShadowConstants(const Shared::ShadowViewConstants::STRUCT& shadowViewConstants)
{
    globalConstantState.shadowViewConstants = shadowViewConstants;
    globalConstantState.shadowViewConstantsDirty.bits = 0x3;
}

//------------------------------------------------------------------------------
/**
*/
void
FlushUpdates(const CoreGraphics::CmdBufferId buf, const CoreGraphics::QueueType queue)
{
    CoreGraphics::PipelineStage sourceStage = queue == CoreGraphics::GraphicsQueueType ? CoreGraphics::PipelineStage::AllShadersRead : CoreGraphics::PipelineStage::ComputeShaderRead;
    IndexT bufferedFrameIndex = CoreGraphics::GetBufferedFrameIndex();

    uint bits = queue == CoreGraphics::GraphicsQueueType ? 0x1 : 0x2;
    if (globalConstantState.tickParamsDirty.bits & bits)
    {
        CoreGraphics::CmdBarrier(buf, sourceStage, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::BarrierDomain::Global,
        {
            {
                CoreGraphics::GetConstantBuffer(bufferedFrameIndex),
                CoreGraphics::BufferSubresourceInfo(globalConstantState.tickCboOffset, sizeof(globalConstantState.tickParams))
            }
        });
        CoreGraphics::CmdUpdateBuffer(buf, CoreGraphics::GetConstantBuffer(bufferedFrameIndex), globalConstantState.tickCboOffset, sizeof(globalConstantState.tickParams), &globalConstantState.tickParams);
        CoreGraphics::CmdBarrier(buf, CoreGraphics::PipelineStage::TransferWrite, sourceStage, CoreGraphics::BarrierDomain::Global,
        {
            {
                CoreGraphics::GetConstantBuffer(bufferedFrameIndex),
                CoreGraphics::BufferSubresourceInfo(globalConstantState.tickCboOffset, sizeof(globalConstantState.tickParams))

            }
        });
        globalConstantState.tickParamsDirty.bits &= ~bits;
    }

    if (globalConstantState.viewConstantsDirty.bits & bits)
    {
        CoreGraphics::CmdBarrier(buf, sourceStage, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::BarrierDomain::Global,
        {
            {
                CoreGraphics::GetConstantBuffer(bufferedFrameIndex),
                CoreGraphics::BufferSubresourceInfo(globalConstantState.viewCboOffset, sizeof(globalConstantState.viewConstants))
            }
        });
        CoreGraphics::CmdUpdateBuffer(buf, CoreGraphics::GetConstantBuffer(bufferedFrameIndex), globalConstantState.viewCboOffset, sizeof(globalConstantState.viewConstants), &globalConstantState.viewConstants);
        CoreGraphics::CmdBarrier(buf, CoreGraphics::PipelineStage::TransferWrite, sourceStage, CoreGraphics::BarrierDomain::Global,
        {
            {
                CoreGraphics::GetConstantBuffer(bufferedFrameIndex),
                CoreGraphics::BufferSubresourceInfo(globalConstantState.viewCboOffset, sizeof(globalConstantState.viewConstants))
            }
        });
        globalConstantState.viewConstantsDirty.bits &= ~bits;
    }

    if (globalConstantState.shadowViewConstantsDirty.bits & bits)
    {
        CoreGraphics::CmdBarrier(buf, sourceStage, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::BarrierDomain::Global,
        {
            {
                CoreGraphics::GetConstantBuffer(bufferedFrameIndex),
                CoreGraphics::BufferSubresourceInfo(globalConstantState.shadowViewCboOffset, sizeof(globalConstantState.shadowViewConstants))
            }
        });
        CoreGraphics::CmdUpdateBuffer(buf, CoreGraphics::GetConstantBuffer(bufferedFrameIndex), globalConstantState.shadowViewCboOffset, sizeof(globalConstantState.shadowViewConstants), &globalConstantState.shadowViewConstants);
        CoreGraphics::CmdBarrier(buf, CoreGraphics::PipelineStage::TransferWrite, sourceStage, CoreGraphics::BarrierDomain::Global,
        {
            {
                CoreGraphics::GetConstantBuffer(bufferedFrameIndex),
                CoreGraphics::BufferSubresourceInfo(globalConstantState.shadowViewCboOffset, sizeof(globalConstantState.shadowViewConstants))
            }
        });
        globalConstantState.shadowViewConstantsDirty.bits &= ~bits;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
GetOffsets(uint64_t& tickOffset, uint64_t& viewOffset, uint64_t& shadowOffset)
{
    tickOffset = globalConstantState.tickCboOffset;
    viewOffset = globalConstantState.viewCboOffset;
    shadowOffset = globalConstantState.shadowViewCboOffset;
}

//------------------------------------------------------------------------------
/**
*/
const Shared::PerTickParams::STRUCT&
GetTickParams()
{
    return globalConstantState.tickParams;
}

//------------------------------------------------------------------------------
/**
*/
const Shared::ViewConstants::STRUCT&
GetViewConstants()
{
    return globalConstantState.viewConstants;
}

//------------------------------------------------------------------------------
/**
*/
const Shared::ShadowViewConstants::STRUCT&
GetShadowViewConstants()
{
    return globalConstantState.shadowViewConstants;
}

//------------------------------------------------------------------------------
/**
*/
void
SetGlobalEnvironmentTextures(const CoreGraphics::TextureId& env, const CoreGraphics::TextureId& irr, const SizeT numMips)
{
    globalConstantState.tickParams.EnvironmentMap = CoreGraphics::TextureGetBindlessHandle(env);
    globalConstantState.tickParams.IrradianceMap = CoreGraphics::TextureGetBindlessHandle(irr);
    globalConstantState.tickParams.NumEnvMips = numMips;
}

//------------------------------------------------------------------------------
/**
*/
void
SetupBufferConstants()
{
    globalConstantState.tickParams.NormalBuffer = TextureGetBindlessHandle(FrameScript_default::Texture_NormalBuffer());
    globalConstantState.tickParams.SpecularBuffer = TextureGetBindlessHandle(FrameScript_default::Texture_SpecularBuffer());
    globalConstantState.tickParams.DepthBuffer = TextureGetBindlessHandle(FrameScript_default::Texture_ZBuffer());
    globalConstantState.tickParams.DepthBufferCopy = TextureGetBindlessHandle(FrameScript_default::Texture_Depth());
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ResourceTableId
GetFrameResourceTable(uint32_t bufferIndex)
{
    return globalConstantState.frameResourceTables[bufferIndex];
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ResourceTableId
GetTickResourceTable(uint32_t bufferIndex)
{
    return globalConstantState.tickResourceTables[bufferIndex];
}

} // namespace Graphics
