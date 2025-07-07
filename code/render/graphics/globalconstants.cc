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

    Shared::PerTickParams tickParams;

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

    Shared::ViewConstants viewConstants;
    Shared::ShadowViewConstants shadowViewConstants;
    DirtySet viewConstantsDirty, shadowViewConstantsDirty;
} state;

//------------------------------------------------------------------------------
/**
*/
void
CreateGlobalConstants(const GlobalConstantsCreateInfo& info)
{
    // create shader state for textures, and fetch variables
    CoreGraphics::ShaderId shader = CoreGraphics::ShaderGet("shd:system_shaders/shared.fxb"_atm);

    state.tableLayout = CoreGraphics::ShaderGetResourcePipeline(shader);

    state.frameResourceTables.Resize(CoreGraphics::GetNumBufferedFrames());
    state.tickResourceTables.Resize(CoreGraphics::GetNumBufferedFrames());
    IndexT i;
    for (i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
    {
        state.frameResourceTables[i] = CoreGraphics::ShaderCreateResourceTable(shader, NEBULA_FRAME_GROUP, state.frameResourceTables.Size());
        CoreGraphics::ObjectSetName(state.frameResourceTables[i], "Main Frame Group Descriptor");

        state.tickResourceTables[i] = CoreGraphics::ShaderCreateResourceTable(shader, NEBULA_TICK_GROUP, state.tickResourceTables.Size());
        CoreGraphics::ObjectSetName(state.tickResourceTables[i], "Main Tick Group Descriptor");
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyGlobalConstants()
{
    for (IndexT i = 0; i < state.frameResourceTables.Size(); i++)
    {
        CoreGraphics::DestroyResourceTable(state.frameResourceTables[i]);
        CoreGraphics::DestroyResourceTable(state.tickResourceTables[i]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AllocateGlobalConstants()
{
    // Allocate memory for the per-tick, per-view and shadow view matrices
    state.tickCboOffset = CoreGraphics::AllocateConstantBufferMemory(sizeof(Shared::PerTickParams));
    state.viewCboOffset = CoreGraphics::AllocateConstantBufferMemory(sizeof(Shared::ViewConstants));
    state.shadowViewCboOffset = CoreGraphics::AllocateConstantBufferMemory(sizeof(Shared::ShadowViewConstants));
    IndexT bufferedFrameIndex = CoreGraphics::GetBufferedFrameIndex();

    // Bind tables with memory allocated
    ResourceTableSetConstantBuffer(state.frameResourceTables[bufferedFrameIndex], { CoreGraphics::GetConstantBuffer(bufferedFrameIndex), Shared::Table_Frame::ViewConstants_SLOT, 0, sizeof(Shared::ViewConstants), state.viewCboOffset });
    ResourceTableSetConstantBuffer(state.frameResourceTables[bufferedFrameIndex], { CoreGraphics::GetConstantBuffer(bufferedFrameIndex), Shared::Table_Frame::ShadowViewConstants_SLOT, 0, sizeof(Shared::ShadowViewConstants), state.shadowViewCboOffset });
    ResourceTableCommitChanges(state.frameResourceTables[bufferedFrameIndex]);

    // Update tick resource tables
    ResourceTableSetConstantBuffer(state.tickResourceTables[bufferedFrameIndex], { CoreGraphics::GetConstantBuffer(bufferedFrameIndex), Shared::Table_Tick::PerTickParams_SLOT, 0, sizeof(Shared::PerTickParams), state.tickCboOffset });
    ResourceTableCommitChanges(state.tickResourceTables[bufferedFrameIndex]);
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateTickParams(const Shared::PerTickParams& tickParams)
{
    state.tickParams = tickParams;
    state.tickParamsDirty.bits = 0x3;
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateViewConstants(const Shared::ViewConstants& viewConstants)
{
    state.viewConstants = viewConstants;
    state.viewConstantsDirty.bits = 0x3;
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateShadowConstants(const Shared::ShadowViewConstants& shadowViewConstants)
{
    state.shadowViewConstants = shadowViewConstants;
    state.shadowViewConstantsDirty.bits = 0x3;
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
    if (state.tickParamsDirty.bits & bits)
    {
        CoreGraphics::CmdBarrier(buf, sourceStage, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::BarrierDomain::Global,
        {
            {
                CoreGraphics::GetConstantBuffer(bufferedFrameIndex),
                CoreGraphics::BufferSubresourceInfo(state.tickCboOffset, sizeof(state.tickParams))
            }
        });
        CoreGraphics::CmdUpdateBuffer(buf, CoreGraphics::GetConstantBuffer(bufferedFrameIndex), state.tickCboOffset, sizeof(state.tickParams), &state.tickParams);
        CoreGraphics::CmdBarrier(buf, CoreGraphics::PipelineStage::TransferWrite, sourceStage, CoreGraphics::BarrierDomain::Global,
        {
            {
                CoreGraphics::GetConstantBuffer(bufferedFrameIndex),
                CoreGraphics::BufferSubresourceInfo(state.tickCboOffset, sizeof(state.tickParams))

            }
        });
        state.tickParamsDirty.bits &= ~bits;
    }

    if (state.viewConstantsDirty.bits & bits)
    {
        CoreGraphics::CmdBarrier(buf, sourceStage, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::BarrierDomain::Global,
        {
            {
                CoreGraphics::GetConstantBuffer(bufferedFrameIndex),
                CoreGraphics::BufferSubresourceInfo(state.viewCboOffset, sizeof(state.viewConstants))
            }
        });
        CoreGraphics::CmdUpdateBuffer(buf, CoreGraphics::GetConstantBuffer(bufferedFrameIndex), state.viewCboOffset, sizeof(state.viewConstants), &state.viewConstants);
        CoreGraphics::CmdBarrier(buf, CoreGraphics::PipelineStage::TransferWrite, sourceStage, CoreGraphics::BarrierDomain::Global,
        {
            {
                CoreGraphics::GetConstantBuffer(bufferedFrameIndex),
                CoreGraphics::BufferSubresourceInfo(state.viewCboOffset, sizeof(state.viewConstants))
            }
        });
        state.viewConstantsDirty.bits &= ~bits;
    }

    if (state.shadowViewConstantsDirty.bits & bits)
    {
        CoreGraphics::CmdBarrier(buf, sourceStage, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::BarrierDomain::Global,
        {
            {
                CoreGraphics::GetConstantBuffer(bufferedFrameIndex),
                CoreGraphics::BufferSubresourceInfo(state.shadowViewCboOffset, sizeof(state.shadowViewConstants))
            }
        });
        CoreGraphics::CmdUpdateBuffer(buf, CoreGraphics::GetConstantBuffer(bufferedFrameIndex), state.shadowViewCboOffset, sizeof(state.shadowViewConstants), &state.shadowViewConstants);
        CoreGraphics::CmdBarrier(buf, CoreGraphics::PipelineStage::TransferWrite, sourceStage, CoreGraphics::BarrierDomain::Global,
        {
            {
                CoreGraphics::GetConstantBuffer(bufferedFrameIndex),
                CoreGraphics::BufferSubresourceInfo(state.shadowViewCboOffset, sizeof(state.shadowViewConstants))
            }
        });
        state.shadowViewConstantsDirty.bits &= ~bits;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
GetOffsets(uint64_t& tickOffset, uint64_t& viewOffset, uint64_t& shadowOffset)
{
    tickOffset = state.tickCboOffset;
    viewOffset = state.viewCboOffset;
    shadowOffset = state.shadowViewCboOffset;
}

//------------------------------------------------------------------------------
/**
*/
const Shared::PerTickParams&
GetTickParams()
{
    return state.tickParams;
}

//------------------------------------------------------------------------------
/**
*/
const Shared::ViewConstants&
GetViewConstants()
{
    return state.viewConstants;
}

//------------------------------------------------------------------------------
/**
*/
const Shared::ShadowViewConstants&
GetShadowViewConstants()
{
    return state.shadowViewConstants;
}

//------------------------------------------------------------------------------
/**
*/
void
SetGlobalEnvironmentTextures(const CoreGraphics::TextureId& env, const CoreGraphics::TextureId& irr, const SizeT numMips)
{
    state.tickParams.EnvironmentMap = CoreGraphics::TextureGetBindlessHandle(env);
    state.tickParams.IrradianceMap = CoreGraphics::TextureGetBindlessHandle(irr);
    state.tickParams.NumEnvMips = numMips;
}

//------------------------------------------------------------------------------
/**
*/
void
SetupBufferConstants()
{
    state.tickParams.NormalBuffer = TextureGetBindlessHandle(FrameScript_default::Texture_NormalBuffer());
    state.tickParams.SpecularBuffer = TextureGetBindlessHandle(FrameScript_default::Texture_SpecularBuffer());
    state.tickParams.DepthBuffer = TextureGetBindlessHandle(FrameScript_default::Texture_ZBuffer());
    state.tickParams.DepthBufferCopy = TextureGetBindlessHandle(FrameScript_default::Texture_Depth());
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ResourceTableId
GetFrameResourceTable(uint32_t bufferIndex)
{
    return state.frameResourceTables[bufferIndex];
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ResourceTableId
GetTickResourceTable(uint32_t bufferIndex)
{
    return state.tickResourceTables[bufferIndex];
}

} // namespace Graphics
