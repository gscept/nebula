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
    IndexT tickCboOffset, viewCboOffset, shadowViewCboOffset;

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
    CoreGraphics::ShaderId shader = CoreGraphics::ShaderGet("shd:shared.fxb"_atm);

    state.tableLayout = CoreGraphics::ShaderGetResourcePipeline(shader);

    state.frameResourceTablesGraphics.Resize(CoreGraphics::GetNumBufferedFrames());
    state.frameResourceTablesCompute.Resize(CoreGraphics::GetNumBufferedFrames());
    state.tickResourceTablesGraphics.Resize(CoreGraphics::GetNumBufferedFrames());
    state.tickResourceTablesCompute.Resize(CoreGraphics::GetNumBufferedFrames());
    IndexT i;
    for (i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
    {
        state.frameResourceTablesGraphics[i] = CoreGraphics::ShaderCreateResourceTable(shader, NEBULA_FRAME_GROUP, state.frameResourceTablesGraphics.Size());
        CoreGraphics::ObjectSetName(state.frameResourceTablesGraphics[i], "Main Frame Group Graphics Descriptor");
        
        state.frameResourceTablesCompute[i] = CoreGraphics::ShaderCreateResourceTable(shader, NEBULA_FRAME_GROUP, state.frameResourceTablesCompute.Size());
        CoreGraphics::ObjectSetName(state.frameResourceTablesCompute[i], "Main Frame Group Compute Descriptor");

        state.tickResourceTablesGraphics[i] = CoreGraphics::ShaderCreateResourceTable(shader, NEBULA_TICK_GROUP, state.tickResourceTablesGraphics.Size());
        CoreGraphics::ObjectSetName(state.tickResourceTablesGraphics[i], "Main Tick Group Graphics Descriptor");

        state.tickResourceTablesCompute[i] = CoreGraphics::ShaderCreateResourceTable(shader, NEBULA_TICK_GROUP, state.tickResourceTablesCompute.Size());
        CoreGraphics::ObjectSetName(state.tickResourceTablesCompute[i], "Main Tick Group Compute Descriptor");
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
        CoreGraphics::DestroyResourceTable(state.tickResourceTablesGraphics[i]);
        CoreGraphics::DestroyResourceTable(state.tickResourceTablesCompute[i]);
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
    ResourceTableSetConstantBuffer(state.frameResourceTablesGraphics[bufferedFrameIndex], { CoreGraphics::GetGraphicsConstantBuffer(), Shared::Table_Frame::ViewConstants::SLOT, 0, Shared::Table_Frame::ViewConstants::SIZE, (SizeT)state.viewCboOffset });
    ResourceTableSetConstantBuffer(state.frameResourceTablesGraphics[bufferedFrameIndex], { CoreGraphics::GetGraphicsConstantBuffer(), Shared::Table_Frame::ShadowViewConstants::SLOT, 0, Shared::Table_Frame::ShadowViewConstants::SIZE, (SizeT)state.shadowViewCboOffset });
    ResourceTableCommitChanges(state.frameResourceTablesGraphics[bufferedFrameIndex]);

    ResourceTableSetConstantBuffer(state.frameResourceTablesCompute[bufferedFrameIndex], { CoreGraphics::GetComputeConstantBuffer(), Shared::Table_Frame::ViewConstants::SLOT, 0, Shared::Table_Frame::ViewConstants::SIZE, (SizeT)state.viewCboOffset });
    ResourceTableSetConstantBuffer(state.frameResourceTablesCompute[bufferedFrameIndex], { CoreGraphics::GetComputeConstantBuffer(), Shared::Table_Frame::ShadowViewConstants::SLOT, 0, Shared::Table_Frame::ShadowViewConstants::SIZE, (SizeT)state.shadowViewCboOffset });
    ResourceTableCommitChanges(state.frameResourceTablesCompute[bufferedFrameIndex]);

    // Update tick resource tables
    ResourceTableSetConstantBuffer(state.tickResourceTablesGraphics[bufferedFrameIndex], { CoreGraphics::GetGraphicsConstantBuffer(), Shared::Table_Tick::PerTickParams::SLOT, 0, Shared::Table_Tick::PerTickParams::SIZE, (SizeT)state.tickCboOffset });
    ResourceTableCommitChanges(state.tickResourceTablesGraphics[bufferedFrameIndex]);
    ResourceTableSetConstantBuffer(state.tickResourceTablesCompute[bufferedFrameIndex], { CoreGraphics::GetComputeConstantBuffer(), Shared::Table_Tick::PerTickParams::SLOT, 0, Shared::Table_Tick::PerTickParams::SIZE, (SizeT)state.tickCboOffset });
    ResourceTableCommitChanges(state.tickResourceTablesCompute[bufferedFrameIndex]);
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateTickParams(const Shared::PerTickParams& tickParams)
{
    state.tickParams = tickParams;
    state.tickParamsDirty.bits = 0xFF;
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateViewConstants(const Shared::ViewConstants& viewConstants)
{
    state.viewConstants = viewConstants;
    state.viewConstantsDirty.bits = 0xFF;
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateShadowConstants(const Shared::ShadowViewConstants& shadowViewConstants)
{
    state.shadowViewConstants = shadowViewConstants;
    state.shadowViewConstantsDirty.bits = 0xFF;
}

//------------------------------------------------------------------------------
/**
*/
void
FlushUpdates(const CoreGraphics::CmdBufferId buf, const CoreGraphics::QueueType queue)
{
    switch (queue)
    {
        case CoreGraphics::QueueType::GraphicsQueueType:
            if (state.tickParamsDirty.graphicsDirty)
            {
                CoreGraphics::CmdUpdateBuffer(buf, CoreGraphics::GetGraphicsConstantBuffer(), state.tickCboOffset, sizeof(state.tickParams), &state.tickParams);
                CoreGraphics::CmdBarrier(buf, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::PipelineStage::AllShadersRead, CoreGraphics::BarrierDomain::Global,
                {
                    {
                        CoreGraphics::GetGraphicsConstantBuffer(),
                        CoreGraphics::BufferSubresourceInfo(state.tickCboOffset, sizeof(state.tickParams))
                        
                    }
                });
                state.tickParamsDirty.graphicsDirty = false;
            }
            if (state.viewConstantsDirty.graphicsDirty)
            {
                CoreGraphics::CmdBarrier(buf, CoreGraphics::PipelineStage::AllShadersRead, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::BarrierDomain::Global,
                {
                    {
                        CoreGraphics::GetGraphicsConstantBuffer(),
                        CoreGraphics::BufferSubresourceInfo(state.viewCboOffset, sizeof(state.viewConstants))
                    }
                });
                CoreGraphics::CmdUpdateBuffer(buf, CoreGraphics::GetGraphicsConstantBuffer(), state.viewCboOffset, sizeof(state.viewConstants), &state.viewConstants);
                CoreGraphics::CmdBarrier(buf, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::PipelineStage::AllShadersRead, CoreGraphics::BarrierDomain::Global,
                {
                    {
                        CoreGraphics::GetGraphicsConstantBuffer(),
                        CoreGraphics::BufferSubresourceInfo(state.viewCboOffset, sizeof(state.viewConstants))
                    }
                });
                state.viewConstantsDirty.graphicsDirty = false;
            }
            if (state.shadowViewConstantsDirty.graphicsDirty)
            {
                CoreGraphics::CmdUpdateBuffer(buf, CoreGraphics::GetGraphicsConstantBuffer(), state.shadowViewCboOffset, sizeof(state.shadowViewConstants), &state.shadowViewConstants);
                CoreGraphics::CmdBarrier(buf, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::PipelineStage::AllShadersRead, CoreGraphics::BarrierDomain::Global,
                {
                    {
                        CoreGraphics::GetGraphicsConstantBuffer(),
                        CoreGraphics::BufferSubresourceInfo(state.shadowViewCboOffset, sizeof(state.shadowViewConstants))
                    }
                });
                state.shadowViewConstantsDirty.graphicsDirty = false;
            }
            break;
        case CoreGraphics::QueueType::ComputeQueueType:
            if (state.tickParamsDirty.computeDirty)
            {
                CoreGraphics::CmdUpdateBuffer(buf, CoreGraphics::GetComputeConstantBuffer(), state.tickCboOffset, sizeof(state.tickParams), &state.tickParams);
                CoreGraphics::CmdBarrier(buf, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::PipelineStage::ComputeShaderRead, CoreGraphics::BarrierDomain::Global,
                         {
                             {
                                 CoreGraphics::GetComputeConstantBuffer(),
                                 CoreGraphics::BufferSubresourceInfo(state.tickCboOffset, sizeof(state.tickParams))

                             }
                         });
                state.tickParamsDirty.computeDirty = false;
            }
            if (state.viewConstantsDirty.computeDirty)
            {
                CoreGraphics::CmdUpdateBuffer(buf, CoreGraphics::GetComputeConstantBuffer(), state.viewCboOffset, sizeof(state.viewConstants), &state.viewConstants);
                CoreGraphics::CmdBarrier(buf, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::PipelineStage::ComputeShaderRead, CoreGraphics::BarrierDomain::Global,
                        {
                            {
                                CoreGraphics::GetComputeConstantBuffer(),
                                CoreGraphics::BufferSubresourceInfo(state.viewCboOffset, sizeof(state.viewConstants))
                            }
                        });
                state.viewConstantsDirty.computeDirty = false;
            }
            if (state.shadowViewConstantsDirty.computeDirty)
            {
                CoreGraphics::CmdUpdateBuffer(buf, CoreGraphics::GetComputeConstantBuffer(), state.shadowViewCboOffset, sizeof(state.shadowViewConstants), &state.shadowViewConstants);
                CoreGraphics::CmdBarrier(buf, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::PipelineStage::ComputeShaderRead, CoreGraphics::BarrierDomain::Global,
                        {
                            {
                                CoreGraphics::GetComputeConstantBuffer(),
                                CoreGraphics::BufferSubresourceInfo(state.tickCboOffset, sizeof(state.shadowViewConstants))
                            }
                        });
                state.shadowViewConstantsDirty.computeDirty = false;
            }
            break;
    }
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
