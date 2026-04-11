//------------------------------------------------------------------------------
// globalconstants.cc
// (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "globalconstants.h"
#include "coregraphics/shader.h"

#include "frame/default.h"
#include <array>
namespace Graphics
{

struct
{
    Util::FixedArray<CoreGraphics::ResourceTableId> frameResourceTables[(uint)GlobalTables::NumQueues];
    Util::FixedArray<CoreGraphics::ResourceTableId> tickResourceTables[(uint)GlobalTables::NumQueues];
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
    uint64_t tickCboOffset[(uint)GlobalTables::NumQueues], viewCboOffset[(uint)GlobalTables::NumQueues], shadowViewCboOffset[(uint)GlobalTables::NumQueues];

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

    for (uint i = 0; i < (uint)GlobalTables::NumQueues; i++)
    {
        globalConstantState.frameResourceTables[i].Resize(CoreGraphics::GetNumBufferedFrames());
        globalConstantState.tickResourceTables[i].Resize(CoreGraphics::GetNumBufferedFrames());

        IndexT j;
        for (j = 0; j < CoreGraphics::GetNumBufferedFrames(); j++)
        {
            globalConstantState.frameResourceTables[i][j] = CoreGraphics::ShaderCreateResourceTable(
                shader, NEBULA_FRAME_GROUP, globalConstantState.frameResourceTables[i].Size()
            );
            CoreGraphics::ObjectSetName(globalConstantState.frameResourceTables[i][j], "Graphics Queue Frame Group Descriptor");

            globalConstantState.tickResourceTables[i][j] = CoreGraphics::ShaderCreateResourceTable(
                shader, NEBULA_TICK_GROUP, globalConstantState.tickResourceTables[i].Size()
            );
            CoreGraphics::ObjectSetName(globalConstantState.tickResourceTables[i][j], "Graphics Tick Group Descriptor");
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyGlobalConstants()
{
    for (uint i = 0; i < (uint)GlobalTables::NumQueues; i++)
    {
        for (IndexT j = 0; j < globalConstantState.frameResourceTables[i].Size(); j++)
        {
            CoreGraphics::DestroyResourceTable(globalConstantState.frameResourceTables[i][j]);
            CoreGraphics::DestroyResourceTable(globalConstantState.tickResourceTables[i][j]);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AllocateGlobalConstants()
{
    IndexT bufferedFrameIndex = CoreGraphics::GetBufferedFrameIndex();

    // Allocate memory for the per-tick, per-view and shadow view matrices
    for (uint i = 0; i < (uint)GlobalTables::NumQueues; i++)
    {
        globalConstantState.tickCboOffset[i] = CoreGraphics::AllocateConstantBufferMemory(sizeof(Shared::PerTickParams::STRUCT));
        globalConstantState.viewCboOffset[i] = CoreGraphics::AllocateConstantBufferMemory(sizeof(Shared::ViewConstants::STRUCT));
        globalConstantState.shadowViewCboOffset[i] = CoreGraphics::AllocateConstantBufferMemory(sizeof(Shared::ShadowViewConstants::STRUCT));

        CoreGraphics::QueueType queue = i == (uint)GlobalTables::GraphicsQueue ? CoreGraphics::QueueType::GraphicsQueueType : CoreGraphics::QueueType::ComputeQueueType;

         // Bind tables with memory allocated
        ResourceTableSetConstantBuffer(globalConstantState.frameResourceTables[i][bufferedFrameIndex], { CoreGraphics::GetConstantBuffer(bufferedFrameIndex, queue), Shared::ViewConstants::BINDING, 0, sizeof(Shared::ViewConstants::STRUCT), globalConstantState.viewCboOffset[i] });
        ResourceTableSetConstantBuffer(globalConstantState.frameResourceTables[i][bufferedFrameIndex], { CoreGraphics::GetConstantBuffer(bufferedFrameIndex, queue), Shared::ShadowViewConstants::BINDING, 0, sizeof(Shared::ShadowViewConstants::STRUCT), globalConstantState.shadowViewCboOffset[i] });
        ResourceTableCommitChanges(globalConstantState.frameResourceTables[i][bufferedFrameIndex]);

        // Update tick resource tables
        ResourceTableSetConstantBuffer(globalConstantState.tickResourceTables[i][bufferedFrameIndex], { CoreGraphics::GetConstantBuffer(bufferedFrameIndex, queue), Shared::PerTickParams::BINDING, 0, sizeof(Shared::PerTickParams::STRUCT), globalConstantState.tickCboOffset[i] });
        ResourceTableCommitChanges(globalConstantState.tickResourceTables[i][bufferedFrameIndex]);
    }
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
    auto buffer = CoreGraphics::GetConstantBuffer(bufferedFrameIndex, queue);
    GlobalTables table = queue == CoreGraphics::GraphicsQueueType ? GlobalTables::GraphicsQueue : GlobalTables::ComputeQueue;
    if (globalConstantState.tickParamsDirty.bits & bits)
    {
        auto offset = globalConstantState.tickCboOffset[uint(table)];
        CoreGraphics::CmdBarrier(buf, sourceStage, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::BarrierDomain::Global,
        {
            {
                buffer,
                CoreGraphics::BufferSubresourceInfo(offset, sizeof(globalConstantState.tickParams))
            }
        });
        CoreGraphics::CmdUpdateBuffer(buf, buffer, offset, sizeof(globalConstantState.tickParams), &globalConstantState.tickParams);
        CoreGraphics::CmdBarrier(buf, CoreGraphics::PipelineStage::TransferWrite, sourceStage, CoreGraphics::BarrierDomain::Global,
        {
            {
                buffer,
                CoreGraphics::BufferSubresourceInfo(offset, sizeof(globalConstantState.tickParams))

            }
        });
        globalConstantState.tickParamsDirty.bits &= ~bits;
    }

    if (globalConstantState.viewConstantsDirty.bits & bits)
    {
        auto offset = globalConstantState.viewCboOffset[uint(table)];
        CoreGraphics::CmdBarrier(buf, sourceStage, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::BarrierDomain::Global,
        {
            {
                buffer,
                CoreGraphics::BufferSubresourceInfo(offset, sizeof(globalConstantState.viewConstants))
            }
        });
        CoreGraphics::CmdUpdateBuffer(buf, buffer, offset, sizeof(globalConstantState.viewConstants), &globalConstantState.viewConstants);
        CoreGraphics::CmdBarrier(buf, CoreGraphics::PipelineStage::TransferWrite, sourceStage, CoreGraphics::BarrierDomain::Global,
        {
            {
                buffer,
                CoreGraphics::BufferSubresourceInfo(offset, sizeof(globalConstantState.viewConstants))
            }
        });
        globalConstantState.viewConstantsDirty.bits &= ~bits;
    }

    if (globalConstantState.shadowViewConstantsDirty.bits & bits)
    {
        auto offset = globalConstantState.shadowViewCboOffset[uint(table)];
        CoreGraphics::CmdBarrier(buf, sourceStage, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::BarrierDomain::Global,
        {
            {
                buffer,
                CoreGraphics::BufferSubresourceInfo(offset, sizeof(globalConstantState.shadowViewConstants))
            }
        });
        CoreGraphics::CmdUpdateBuffer(buf, buffer, offset, sizeof(globalConstantState.shadowViewConstants), &globalConstantState.shadowViewConstants);
        CoreGraphics::CmdBarrier(buf, CoreGraphics::PipelineStage::TransferWrite, sourceStage, CoreGraphics::BarrierDomain::Global,
        {
            {
                buffer,
                CoreGraphics::BufferSubresourceInfo(offset, sizeof(globalConstantState.shadowViewConstants))
            }
        });
        globalConstantState.shadowViewConstantsDirty.bits &= ~bits;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
GetOffsets(uint64_t& tickOffset, uint64_t& viewOffset, uint64_t& shadowOffset, const GlobalTables table)
{
    tickOffset = globalConstantState.tickCboOffset[uint(table)];
    viewOffset = globalConstantState.viewCboOffset[uint(table)];
    shadowOffset = globalConstantState.shadowViewCboOffset[uint(table)];
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
    globalConstantState.tickParams.Scene = TextureGetBindlessHandle(FrameScript_default::Texture_LightBuffer());
    globalConstantState.tickParams.DepthBuffer = TextureGetBindlessHandle(FrameScript_default::Texture_ZBuffer());
    globalConstantState.tickParams.DepthBufferCopy = TextureGetBindlessHandle(FrameScript_default::Texture_Depth());
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ResourceTableId
GetFrameResourceTable(uint32_t bufferIndex, GlobalTables table)
{
    return globalConstantState.frameResourceTables[uint(table)][bufferIndex];
}

//------------------------------------------------------------------------------
/**
*/
const std::array<CoreGraphics::ResourceTableId, (uint)GlobalTables::NumQueues>
GetFrameResourceTables(uint32_t bufferIndex)
{
    return { globalConstantState.frameResourceTables[uint(GlobalTables::GraphicsQueue)][bufferIndex], globalConstantState.frameResourceTables[uint(GlobalTables::ComputeQueue)][bufferIndex]  };
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ResourceTableId
GetTickResourceTable(uint32_t bufferIndex, GlobalTables table)
{
    return globalConstantState.tickResourceTables[uint(table)][bufferIndex];
}

//------------------------------------------------------------------------------
/**
*/
const std::array<CoreGraphics::ResourceTableId, (uint)GlobalTables::NumQueues>
GetTickResourceTables(uint32_t bufferIndex)
{
    return {globalConstantState.tickResourceTables[uint(GlobalTables::GraphicsQueue)][bufferIndex], globalConstantState.tickResourceTables[uint(GlobalTables::ComputeQueue)][bufferIndex]};
}

//------------------------------------------------------------------------------
/**
*/
const std::array<CoreGraphics::QueueType, (uint)GlobalTables::NumQueues>
GetTableQueues()
{
    return { CoreGraphics::QueueType::GraphicsQueueType, CoreGraphics::QueueType::ComputeQueueType };
}

} // namespace Graphics
