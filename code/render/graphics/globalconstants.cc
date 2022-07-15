//------------------------------------------------------------------------------
// globalconstants.cc
// (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "globalconstants.h"
#include "coregraphics/shader.h"

namespace Graphics
{

struct
{
    Util::FixedArray<CoreGraphics::ResourceTableId> frameResourceTablesGraphics;
    Util::FixedArray<CoreGraphics::ResourceTableId> frameResourceTablesCompute;
    Util::FixedArray<CoreGraphics::ResourceTableId> tickResourceTablesGraphics;
    Util::FixedArray<CoreGraphics::ResourceTableId> tickResourceTablesCompute;
    CoreGraphics::ResourcePipelineId tableLayout;
    IndexT tickParamsCboSlot, viewConstantsSlot, shadowViewConstantsSlot;

    Shared::PerTickParams tickParams;
    IndexT tickCboOffset, viewCboOffset, shadowViewCboOffset;

    Shared::ViewConstants viewConstants;
    Shared::ShadowViewConstants shadowViewConstants;
    bool viewConstantsGfxDirty, shadowViewConstantsGfxDirty, viewConstantsComputeDirty, shadowViewConstantsComputeDirty;
} state;

//------------------------------------------------------------------------------
/**
*/
void
CreateGlobalConstants(const GlobalConstantsCreateInfo& info)
{
    // create shader state for textures, and fetch variables
    CoreGraphics::ShaderId shader = CoreGraphics::ShaderGet("shd:shared.fxb"_atm);

    state.tableLayout = CoreGraphics::ShaderGetResourcePipeline(shader);
    state.tickParamsCboSlot = CoreGraphics::ShaderGetResourceSlot(shader, "PerTickParams");
    state.viewConstantsSlot = CoreGraphics::ShaderGetResourceSlot(shader, "ViewConstants");
    state.shadowViewConstantsSlot = CoreGraphics::ShaderGetResourceSlot(shader, "ShadowViewConstants");

    state.frameResourceTablesGraphics.Resize(CoreGraphics::GetNumBufferedFrames());
    state.frameResourceTablesCompute.Resize(CoreGraphics::GetNumBufferedFrames());
    state.tickResourceTablesGraphics.Resize(CoreGraphics::GetNumBufferedFrames());
    state.tickResourceTablesCompute.Resize(CoreGraphics::GetNumBufferedFrames());
    IndexT i;
    for (i = 0; i < state.frameResourceTablesGraphics.Size(); i++)
    {
        state.frameResourceTablesGraphics[i] = CoreGraphics::ShaderCreateResourceTable(shader, NEBULA_FRAME_GROUP, state.frameResourceTablesGraphics.Size());
        CoreGraphics::ObjectSetName(state.frameResourceTablesGraphics[i], "Main Frame Group Descriptor");
        
        state.frameResourceTablesCompute[i] = CoreGraphics::ShaderCreateResourceTable(shader, NEBULA_FRAME_GROUP, state.frameResourceTablesCompute.Size());
        CoreGraphics::ObjectSetName(state.frameResourceTablesCompute[i], "Main Frame Group Descriptor");

        state.tickResourceTablesGraphics[i] = CoreGraphics::ShaderCreateResourceTable(shader, NEBULA_TICK_GROUP, state.tickResourceTablesGraphics.Size());
        CoreGraphics::ObjectSetName(state.tickResourceTablesGraphics[i], "Main Tick Group Descriptor");

        state.tickResourceTablesCompute[i] = CoreGraphics::ShaderCreateResourceTable(shader, NEBULA_TICK_GROUP, state.tickResourceTablesCompute.Size());
        CoreGraphics::ObjectSetName(state.tickResourceTablesCompute[i], "Main Tick Group Descriptor");
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyGlobalConstants()
{
    for (IndexT i = 0; i < state.frameResourceTablesGraphics.Size(); i++)
    {
        CoreGraphics::DestroyResourceTable(state.frameResourceTablesGraphics[i]);
        CoreGraphics::DestroyResourceTable(state.frameResourceTablesCompute[i]);
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
    ResourceTableSetConstantBuffer(state.frameResourceTablesGraphics[bufferedFrameIndex], { CoreGraphics::GetGraphicsConstantBuffer(), state.viewConstantsSlot, 0, false, false, sizeof(Shared::PerTickParams), (SizeT)state.viewCboOffset });
    ResourceTableSetConstantBuffer(state.frameResourceTablesGraphics[bufferedFrameIndex], { CoreGraphics::GetGraphicsConstantBuffer(), state.shadowViewConstantsSlot, 0, false, false, sizeof(Shared::PerTickParams), (SizeT)state.shadowViewCboOffset });
    ResourceTableCommitChanges(state.frameResourceTablesGraphics[bufferedFrameIndex]);

    ResourceTableSetConstantBuffer(state.frameResourceTablesCompute[bufferedFrameIndex], { CoreGraphics::GetComputeConstantBuffer(), state.viewConstantsSlot, 0, false, false, sizeof(Shared::PerTickParams), (SizeT)state.viewCboOffset });
    ResourceTableSetConstantBuffer(state.frameResourceTablesCompute[bufferedFrameIndex], { CoreGraphics::GetComputeConstantBuffer(), state.shadowViewConstantsSlot, 0, false, false, sizeof(Shared::PerTickParams), (SizeT)state.shadowViewCboOffset });
    ResourceTableCommitChanges(state.frameResourceTablesCompute[bufferedFrameIndex]);

    // Update tick resource tables
    ResourceTableSetConstantBuffer(state.tickResourceTablesGraphics[bufferedFrameIndex], { CoreGraphics::GetGraphicsConstantBuffer(), state.tickParamsCboSlot, 0, false, false, sizeof(Shared::PerTickParams), (SizeT)state.tickCboOffset });
    ResourceTableCommitChanges(state.tickResourceTablesGraphics[bufferedFrameIndex]);
    ResourceTableSetConstantBuffer(state.tickResourceTablesCompute[bufferedFrameIndex], { CoreGraphics::GetComputeConstantBuffer(), state.tickParamsCboSlot, 0, false, false, sizeof(Shared::PerTickParams), (SizeT)state.tickCboOffset });
    ResourceTableCommitChanges(state.tickResourceTablesCompute[bufferedFrameIndex]);

    // Set tick table
    CoreGraphics::SetFrameResourceTableGraphics(state.frameResourceTablesGraphics[bufferedFrameIndex]);
    CoreGraphics::SetFrameResourceTableCompute(state.frameResourceTablesCompute[bufferedFrameIndex]);
    CoreGraphics::SetTickResourceTableGraphics(state.tickResourceTablesGraphics[bufferedFrameIndex]);
    CoreGraphics::SetTickResourceTableCompute(state.tickResourceTablesCompute[bufferedFrameIndex]);
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateViewConstants(const Shared::ViewConstants& viewConstants)
{
    state.viewConstants = viewConstants;
    state.viewConstantsGfxDirty = state.viewConstantsComputeDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateShadowConstants(const Shared::ShadowViewConstants& shadowViewConstants)
{
    state.shadowViewConstants = shadowViewConstants;
    state.shadowViewConstantsGfxDirty = state.shadowViewConstantsComputeDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
void
FlushConstants(const CoreGraphics::CmdBufferId cmdBuf, CoreGraphics::QueueType queue)
{
    // If we have any pending updates to our constant buffers, execute them
    switch (queue)
    {
        case CoreGraphics::GraphicsQueueType:
        {
            CoreGraphics::BufferUpload(cmdBuf, CoreGraphics::GetGraphicsConstantBuffer(), &state.tickParams, 1u, state.tickCboOffset);
            CoreGraphics::BufferUpload(cmdBuf, CoreGraphics::GetGraphicsConstantBuffer(), &state.viewConstants, 1u, (uint)state.viewCboOffset);
            CoreGraphics::BufferUpload(cmdBuf, CoreGraphics::GetGraphicsConstantBuffer(), &state.shadowViewConstants, 1u, (uint)state.shadowViewCboOffset);
            Util::FixedArray<CoreGraphics::BufferBarrierInfo> bufferBarriers =
            {
                { CoreGraphics::GetGraphicsConstantBuffer(), state.tickCboOffset, sizeof(Shared::PerTickParams) },
                { CoreGraphics::GetGraphicsConstantBuffer(), state.viewCboOffset, sizeof(Shared::ViewConstants) },
                { CoreGraphics::GetGraphicsConstantBuffer(), state.shadowViewCboOffset, sizeof(Shared::ShadowViewConstants) }
            };
            CoreGraphics::CmdBarrier(cmdBuf, CoreGraphics::PipelineStage::UniformGraphics, CoreGraphics::PipelineStage::UniformGraphics, CoreGraphics::BarrierDomain::Global, nullptr, bufferBarriers);
            break;
        }
        case CoreGraphics::ComputeQueueType:
        {
            CoreGraphics::BufferUpload(cmdBuf, CoreGraphics::GetComputeConstantBuffer(), &state.tickParams, 1u, state.tickCboOffset);
            CoreGraphics::BufferUpload(cmdBuf, CoreGraphics::GetComputeConstantBuffer(), &state.viewConstants, 1u, (uint)state.viewCboOffset);
            CoreGraphics::BufferUpload(cmdBuf, CoreGraphics::GetComputeConstantBuffer(), &state.shadowViewConstants, 1u, (uint)state.shadowViewCboOffset);
            Util::FixedArray<CoreGraphics::BufferBarrierInfo> bufferBarriers =
            {
                { CoreGraphics::GetComputeConstantBuffer(), state.tickCboOffset, sizeof(Shared::PerTickParams) },
                { CoreGraphics::GetComputeConstantBuffer(), state.viewCboOffset, sizeof(Shared::ViewConstants) },
                { CoreGraphics::GetComputeConstantBuffer(), state.shadowViewCboOffset, sizeof(Shared::ShadowViewConstants) }
            };
            CoreGraphics::CmdBarrier(cmdBuf, CoreGraphics::PipelineStage::UniformCompute, CoreGraphics::PipelineStage::UniformCompute, CoreGraphics::BarrierDomain::Global, nullptr, bufferBarriers);
            break;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
Shared::PerTickParams&
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
SetupBufferConstants(const Ptr<Frame::FrameScript>& frameScript)
{
    state.tickParams.NormalBuffer = TextureGetBindlessHandle(frameScript->GetTexture("NormalBuffer"));
    state.tickParams.SpecularBuffer = TextureGetBindlessHandle(frameScript->GetTexture("SpecularBuffer"));
    state.tickParams.DepthBuffer = TextureGetBindlessHandle(frameScript->GetTexture("ZBuffer"));
    state.tickParams.DepthBufferCopy = TextureGetBindlessHandle(frameScript->GetTexture("ZBufferCopy"));
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ResourceTableId
GetFrameResourceTableGraphics(uint32_t bufferIndex)
{
    return state.frameResourceTablesGraphics[bufferIndex];
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ResourceTableId
GetFrameResourceTableCompute(uint32_t bufferIndex)
{
    return state.frameResourceTablesCompute[bufferIndex];
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ResourceTableId
GetTickResourceTableGraphics(uint32_t bufferIndex)
{
    return state.tickResourceTablesGraphics[bufferIndex];
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ResourceTableId
GetTickResourceTableCompute(uint32_t bufferIndex)
{
    return state.tickResourceTablesCompute[bufferIndex];
}

} // namespace Graphics
