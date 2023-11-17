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
    ResourceTableSetConstantBuffer(state.frameResourceTables[bufferedFrameIndex], { CoreGraphics::GetGraphicsConstantBuffer(bufferedFrameIndex), Shared::Table_Frame::ViewConstants::SLOT, 0, Shared::Table_Frame::ViewConstants::SIZE, (SizeT)state.viewCboOffset });
    ResourceTableSetConstantBuffer(state.frameResourceTables[bufferedFrameIndex], { CoreGraphics::GetGraphicsConstantBuffer(bufferedFrameIndex), Shared::Table_Frame::ShadowViewConstants::SLOT, 0, Shared::Table_Frame::ShadowViewConstants::SIZE, (SizeT)state.shadowViewCboOffset });
    ResourceTableCommitChanges(state.frameResourceTables[bufferedFrameIndex]);

    // Update tick resource tables
    ResourceTableSetConstantBuffer(state.tickResourceTables[bufferedFrameIndex], { CoreGraphics::GetGraphicsConstantBuffer(bufferedFrameIndex), Shared::Table_Tick::PerTickParams::SLOT, 0, Shared::Table_Tick::PerTickParams::SIZE, (SizeT)state.tickCboOffset });
    ResourceTableCommitChanges(state.tickResourceTables[bufferedFrameIndex]);
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
FlushUpdates(const CoreGraphics::CmdBufferId buf)
{
    IndexT bufferedFrameIndex = CoreGraphics::GetBufferedFrameIndex();
    if (state.tickParamsDirty.graphicsDirty)
    {
        CoreGraphics::CmdUpdateBuffer(buf, CoreGraphics::GetGraphicsConstantBuffer(bufferedFrameIndex), state.tickCboOffset, sizeof(state.tickParams), &state.tickParams);
        CoreGraphics::CmdBarrier(buf, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::PipelineStage::AllShadersRead, CoreGraphics::BarrierDomain::Global,
        {
            {
                CoreGraphics::GetGraphicsConstantBuffer(bufferedFrameIndex),
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
                CoreGraphics::GetGraphicsConstantBuffer(bufferedFrameIndex),
                CoreGraphics::BufferSubresourceInfo(state.viewCboOffset, sizeof(state.viewConstants))
            }
        });
        CoreGraphics::CmdUpdateBuffer(buf, CoreGraphics::GetGraphicsConstantBuffer(bufferedFrameIndex), state.viewCboOffset, sizeof(state.viewConstants), &state.viewConstants);
        CoreGraphics::CmdBarrier(buf, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::PipelineStage::AllShadersRead, CoreGraphics::BarrierDomain::Global,
        {
            {
                CoreGraphics::GetGraphicsConstantBuffer(bufferedFrameIndex),
                CoreGraphics::BufferSubresourceInfo(state.viewCboOffset, sizeof(state.viewConstants))
            }
        });
        state.viewConstantsDirty.graphicsDirty = false;
    }
    if (state.shadowViewConstantsDirty.graphicsDirty)
    {
        CoreGraphics::CmdUpdateBuffer(buf, CoreGraphics::GetGraphicsConstantBuffer(bufferedFrameIndex), state.shadowViewCboOffset, sizeof(state.shadowViewConstants), &state.shadowViewConstants);
        CoreGraphics::CmdBarrier(buf, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::PipelineStage::AllShadersRead, CoreGraphics::BarrierDomain::Global,
        {
            {
                CoreGraphics::GetGraphicsConstantBuffer(bufferedFrameIndex),
                CoreGraphics::BufferSubresourceInfo(state.shadowViewCboOffset, sizeof(state.shadowViewConstants))
            }
        });
        state.shadowViewConstantsDirty.graphicsDirty = false;
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
    state.tickParams.DepthBufferCopy = TextureGetBindlessHandle(frameScript->GetTexture("Depth"));
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
