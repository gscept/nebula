//------------------------------------------------------------------------------
//  histogramcontext.cc
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "frame/framescript.h"
#include "graphics/graphicsserver.h"
#include "histogramcontext.h"
#include "frame/framesubgraph.h"
#include "frame/framecode.h"

#include "graphics/globalconstants.h"

#include "downsample_cs_min.h"
#include "histogram_cs.h"
#include "shared.h"
#include "core/cvar.h"

namespace PostEffects
{

__ImplementPluginContext(PostEffects::HistogramContext);
struct
{
    CoreGraphics::ShaderId histogramShader;
    CoreGraphics::ShaderProgramId histogramCategorizeProgram;
    CoreGraphics::BufferId histogramCounters;
    CoreGraphics::BufferId histogramConstants;
    CoreGraphics::BufferId histogramClearCountersBuffer;
    CoreGraphics::ResourceTableId histogramResourceTable;
    Util::FixedArray<CoreGraphics::BufferId> histogramReadback;

    CoreGraphics::ShaderId downsampleShader;
    CoreGraphics::ShaderProgramId downsampleProgram;
    CoreGraphics::BufferId downsampleCounter;
    CoreGraphics::BufferId downsampleConstants;
    CoreGraphics::ResourceTableId downsampleResourceTable;

    CoreGraphics::TextureDimensions sourceTextureDimensions;
    CoreGraphics::TextureId sourceTexture;
    IndexT sourceTextureBinding;
    Util::FixedArray<CoreGraphics::TextureViewId> downsampledColorBufferViews;

    Memory::ArenaAllocator<sizeof(Frame::FrameCode) * 4> frameOpAllocator;

    Math::float2 offset, size;
    int mip;

    float logLuminanceRange;
    Core::CVar* minLuminance;
    float logMinLuminance;

    float previousLum;

} histogramState;

//------------------------------------------------------------------------------
/**
*/
HistogramContext::HistogramContext()
{
}

//------------------------------------------------------------------------------
/**
*/
HistogramContext::~HistogramContext()
{
}

//------------------------------------------------------------------------------
/**
*/
void
HistogramContext::Create()
{
    __CreatePluginContext();
    __bundle.OnUpdateViewResources = HistogramContext::UpdateViewResources;
    __bundle.OnWindowResized = HistogramContext::WindowResized;
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    histogramState.minLuminance = Core::CVarCreate(Core::CVar_Float, "r_min_luminance", "0.1", "Minimum luminance, used for auto exposure");
    
    histogramState.histogramShader = CoreGraphics::ShaderGet("shd:histogram_cs.fxb");
    histogramState.histogramCategorizeProgram = CoreGraphics::ShaderGetProgram(histogramState.histogramShader, CoreGraphics::ShaderFeatureFromString("HistogramCategorize"));

    CoreGraphics::BufferCreateInfo bufInfo;
    bufInfo.elementSize = sizeof(HistogramCs::HistogramBuffer);
    bufInfo.size = 1;
    bufInfo.usageFlags = CoreGraphics::ReadWriteBuffer | CoreGraphics::TransferBufferSource;
    bufInfo.mode = CoreGraphics::DeviceLocal;
    bufInfo.queueSupport = CoreGraphics::GraphicsQueueSupport;
    uint initDatas[256] = { 0 };
    bufInfo.data = initDatas;
    bufInfo.dataSize = sizeof(initDatas);
    histogramState.histogramCounters = CoreGraphics::CreateBuffer(bufInfo);

    // create clear buffer
    bufInfo.usageFlags = CoreGraphics::TransferBufferSource;
    histogramState.histogramClearCountersBuffer = CoreGraphics::CreateBuffer(bufInfo);

    // setup readback buffers
    bufInfo.mode = CoreGraphics::HostLocal;
    bufInfo.data = nullptr;
    bufInfo.dataSize = 0;
    bufInfo.usageFlags = CoreGraphics::TransferBufferDestination;
    histogramState.histogramReadback.Resize(CoreGraphics::GetNumBufferedFrames());
    for (IndexT i = 0; i < histogramState.histogramReadback.Size(); i++)
    {
        histogramState.histogramReadback[i] = CoreGraphics::CreateBuffer(bufInfo);
    }

    bufInfo.elementSize = sizeof(HistogramCs::HistogramConstants);
    bufInfo.mode = CoreGraphics::HostCached; // lazy but meh
    bufInfo.usageFlags = CoreGraphics::ConstantBuffer;
    bufInfo.data = nullptr;
    bufInfo.dataSize = 0;
    histogramState.histogramConstants = CoreGraphics::CreateBuffer(bufInfo);

    histogramState.histogramResourceTable = CoreGraphics::ShaderCreateResourceTable(histogramState.histogramShader, NEBULA_BATCH_GROUP);
    CoreGraphics::ResourceTableSetRWBuffer(histogramState.histogramResourceTable, {
        histogramState.histogramCounters,
        CoreGraphics::ShaderGetResourceSlot(histogramState.histogramShader, "HistogramBuffer"),
        0,
        false,
        false,
        CoreGraphics::BufferGetByteSize(histogramState.histogramCounters),
        0
    });
    CoreGraphics::ResourceTableSetConstantBuffer(histogramState.histogramResourceTable, {
        histogramState.histogramConstants,
        CoreGraphics::ShaderGetResourceSlot(histogramState.histogramShader, "HistogramConstants"),
        0,
        false,
        false,
        CoreGraphics::BufferGetByteSize(histogramState.histogramConstants),
        0
    });
    CoreGraphics::ResourceTableCommitChanges(histogramState.histogramResourceTable);

    histogramState.downsampleShader = CoreGraphics::ShaderGet("shd:downsample_cs_avg.fxb");
    histogramState.downsampleProgram = CoreGraphics::ShaderGetProgram(histogramState.downsampleShader, CoreGraphics::ShaderFeatureFromString("Downsample"));

    // create counter for downsample shader
    bufInfo.elementSize = sizeof(uint);
    bufInfo.size = 6;
    bufInfo.usageFlags = CoreGraphics::ReadWriteBuffer;
    bufInfo.mode = CoreGraphics::DeviceLocal;
    bufInfo.queueSupport = CoreGraphics::GraphicsQueueSupport;
    uint initData[6] = { 0 };
    bufInfo.data = &initData;
    bufInfo.dataSize = sizeof(initData);
    histogramState.downsampleCounter = CoreGraphics::CreateBuffer(bufInfo);

    bufInfo.elementSize = sizeof(DownsampleCsMin::DownsampleUniforms);
    bufInfo.mode = CoreGraphics::HostCached;
    bufInfo.usageFlags = CoreGraphics::ConstantBuffer;
    bufInfo.data = nullptr;
    bufInfo.dataSize = 0;
    histogramState.downsampleConstants = CoreGraphics::CreateBuffer(bufInfo);

    histogramState.downsampleResourceTable = CoreGraphics::ShaderCreateResourceTable(histogramState.downsampleShader, NEBULA_BATCH_GROUP);

    CoreGraphics::ResourceTableSetRWBuffer(histogramState.downsampleResourceTable, {
        histogramState.downsampleCounter,
        CoreGraphics::ShaderGetResourceSlot(histogramState.downsampleShader, "AtomicCounter"),
        0,
        false,
        false,
        CoreGraphics::BufferGetByteSize(histogramState.downsampleCounter),
        0
    });
    CoreGraphics::ResourceTableSetConstantBuffer(histogramState.downsampleResourceTable, {
        histogramState.downsampleConstants,
        CoreGraphics::ShaderGetResourceSlot(histogramState.downsampleShader, "DownsampleUniforms"),
        0,
        false,
        false,
        CoreGraphics::BufferGetByteSize(histogramState.downsampleConstants),
        0
    });
    CoreGraphics::ResourceTableCommitChanges(histogramState.downsampleResourceTable);
}

//------------------------------------------------------------------------------
/**
*/
void
HistogramContext::Discard()
{
    histogramState.frameOpAllocator.Release();
}

//------------------------------------------------------------------------------
/**
*/
void
HistogramContext::SetWindow(const Math::float2 offset, Math::float2 size, int mip)
{
    SizeT mippedWidth = histogramState.sourceTextureDimensions.width >> mip;
    SizeT mippedHeight = histogramState.sourceTextureDimensions.height >> mip;
    histogramState.offset = { mippedWidth * offset.x, mippedHeight * offset.y };
    histogramState.size = { mippedWidth * size.x, mippedHeight * size.y };
    histogramState.mip = mip;

    HistogramContext::UpdateConstants();
}

//------------------------------------------------------------------------------
/**
*/
void
HistogramContext::Setup(const Ptr<Frame::FrameScript>& script)
{
    histogramState.sourceTexture = script->GetTexture("LightBuffer");
    CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(histogramState.sourceTexture);

    // setup views for output and bind
    SizeT numMips = CoreGraphics::TextureGetNumMips(histogramState.sourceTexture);
    histogramState.downsampledColorBufferViews.Resize(numMips);
    for (IndexT i = 0; i < numMips; i++)
    {
        CoreGraphics::TextureViewCreateInfo info;
        info.tex = histogramState.sourceTexture;
        info.startMip = i;
        info.numMips = 1;
        info.startLayer = 0;
        info.numLayers = 1;
        info.format = CoreGraphics::TextureGetPixelFormat(histogramState.sourceTexture);
        histogramState.downsampledColorBufferViews[i] = CoreGraphics::CreateTextureView(info);

        if (i == 5)
        {
            CoreGraphics::ResourceTableSetRWTexture(histogramState.downsampleResourceTable,
            {
                    histogramState.downsampledColorBufferViews[i],
                    CoreGraphics::ShaderGetResourceSlot(histogramState.downsampleShader, "Output6"),
                    0,
                    CoreGraphics::InvalidSamplerId,
                    false,
                    false
            });
        }
        else
        {
            CoreGraphics::ResourceTableSetRWTexture(histogramState.downsampleResourceTable,
            {
                    histogramState.downsampledColorBufferViews[i],
                    CoreGraphics::ShaderGetResourceSlot(histogramState.downsampleShader, "Output"),
                    i,
                    CoreGraphics::InvalidSamplerId,
                    false,
                    false
            });
        }
    }
    CoreGraphics::ResourceTableCommitChanges(histogramState.downsampleResourceTable);

    histogramState.sourceTextureBinding = CoreGraphics::ShaderGetResourceSlot(histogramState.histogramShader, "ColorSource");
    histogramState.sourceTextureDimensions = dims;
    CoreGraphics::ResourceTableSetTexture(histogramState.histogramResourceTable,
    {
            histogramState.sourceTexture,
            histogramState.sourceTextureBinding,
            0,
            CoreGraphics::InvalidSamplerId,
            false,
            false
    });
    CoreGraphics::ResourceTableCommitChanges(histogramState.histogramResourceTable);

    uint dispatchX = (dims.width - 1) / 64;
    uint dispatchY = (dims.height - 1) / 64;

    histogramState.logLuminanceRange = Math::log2(65000.0f); // R11G11B10 maxes out around 65k (https://www.khronos.org/opengl/wiki/Small_Float_Formats)
    //histogramState.logMinLuminance = Math::log2(10.0f);

    DownsampleCsMin::DownsampleUniforms constants;
    constants.Mips = numMips-1;
    constants.NumGroups = (dispatchX + 1) * (dispatchY + 1);
    BufferUpdate(histogramState.downsampleConstants, constants, 0);

    histogramState.offset.x = 0;
    histogramState.offset.y = 0;
    histogramState.size.x = 1.0f;
    histogramState.size.y = 1.0f;

    // Construct subgraph
    Frame::FrameCode* downsample = histogramState.frameOpAllocator.Alloc<Frame::FrameCode>();
    downsample->SetName("Histogram Downsample");
    downsample->textureDeps.Add(histogramState.sourceTexture,
                                {
                                    "Histogram Downsample"
                                    , CoreGraphics::PipelineStage::ComputeShaderWrite
                                    , CoreGraphics::ImageSubresourceInfo::ColorNoMipNoLayer()
                                });

    downsample->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        CoreGraphics::CmdSetShaderProgram(cmdBuf, histogramState.downsampleProgram, false);
        CoreGraphics::CmdSetResourceTable(cmdBuf, histogramState.downsampleResourceTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
        uint dispatchX = (histogramState.sourceTextureDimensions.width - 1) / 64;
        uint dispatchY = (histogramState.sourceTextureDimensions.height - 1) / 64;
        CoreGraphics::CmdDispatch(cmdBuf, dispatchX + 1, dispatchY + 1, 1);
    };

    Frame::FrameCode* bucket = histogramState.frameOpAllocator.Alloc<Frame::FrameCode>();
    bucket->SetName("Histogram Bucket");
    bucket->bufferDeps.Add(histogramState.histogramCounters,
                            {
                                "Histogram Downsample"
                                , CoreGraphics::PipelineStage::ComputeShaderWrite
                                , CoreGraphics::BufferSubresourceInfo()
                            });
    bucket->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        CoreGraphics::CmdSetShaderProgram(cmdBuf, histogramState.histogramCategorizeProgram, false);
        CoreGraphics::CmdSetResourceTable(cmdBuf, histogramState.histogramResourceTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
        int groupsX = (histogramState.size.x - histogramState.offset.x) / 256;
        int groupsY = (histogramState.size.y - histogramState.offset.y);
        CoreGraphics::CmdDispatch(cmdBuf, groupsX, groupsY, 1);
    };

    Frame::FrameCode* copy = histogramState.frameOpAllocator.Alloc<Frame::FrameCode>();
    copy->SetName("Histogram Copy To Readback");
    copy->bufferDeps.Add(histogramState.histogramCounters,
                            {
                                "Histogram Downsample"
                                , CoreGraphics::PipelineStage::TransferRead
                                , CoreGraphics::BufferSubresourceInfo()
                            });
    copy->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        CoreGraphics::BarrierPush(
        cmdBuf,
        CoreGraphics::PipelineStage::HostRead,
        CoreGraphics::PipelineStage::TransferWrite,
        CoreGraphics::BarrierDomain::Global,
        {
            CoreGraphics::BufferBarrierInfo
            {
                histogramState.histogramReadback[bufferIndex],
                0, NEBULA_WHOLE_BUFFER_SIZE
            },
        });

        CoreGraphics::BufferCopy from, to;
        from.offset = 0;
        to.offset = 0;
        CoreGraphics::CmdCopy(
            cmdBuf,
            histogramState.histogramCounters, { from },
            histogramState.histogramReadback[bufferIndex], { to }, CoreGraphics::BufferGetByteSize(histogramState.histogramCounters));

        CoreGraphics::BarrierPop(cmdBuf);
    };

    Frame::FrameCode* clear = histogramState.frameOpAllocator.Alloc<Frame::FrameCode>();
    clear->SetName("Histogram Clear");
    clear->bufferDeps.Add(histogramState.histogramCounters,
                            {
                                "Histogram Clear"
                                , CoreGraphics::PipelineStage::TransferWrite
                                , CoreGraphics::BufferSubresourceInfo()
                            });
    clear->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        // Clear histogram counters
        uint initDatas[255] = { 0 };
        CoreGraphics::BufferUpload(cmdBuf, histogramState.histogramCounters, initDatas, 255, 0);
    };

    // Add subgraph
    Frame::AddSubgraph("Histogram", { downsample, bucket, copy, clear });
}

//------------------------------------------------------------------------------
/**
*/
void
HistogramContext::UpdateViewResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
    CoreGraphics::BufferInvalidate(histogramState.histogramReadback[ctx.bufferIndex], 0, NEBULA_WHOLE_BUFFER_SIZE);
    int* buf = CoreGraphics::BufferMap<int>(histogramState.histogramReadback[ctx.bufferIndex]);

    if (Core::CVarModified(histogramState.minLuminance))
        HistogramContext::UpdateConstants();

    int numPixels = histogramState.size.x * histogramState.size.y;
    int numBlackPixels = buf[0];
    int numInterestingPixels = Math::max(numPixels - numBlackPixels, 1);

    // sum up all buckets and use their index as a weight
    // meaning bucket 1 are low values, and bucket 255 are high values
    int totalLum = 0;
    for (int i = 1; i < 255; i++)
    {
        totalLum += i * buf[i];
    }

    // we only want to divide by the pixels which are non-black
    float averageLum = (totalLum / float(numInterestingPixels)) - 1.0f;

    // convert from log space back to luminance
    averageLum = Math::exp2(((averageLum / 254.0f) * histogramState.logLuminanceRange) + histogramState.logMinLuminance);

    Timing::Time time = FrameSync::FrameSyncTimer::Instance()->GetFrameTime();
    float lum = histogramState.previousLum + (averageLum - histogramState.previousLum) * time;
    histogramState.previousLum = lum;

    Shared::ViewConstants viewConstants = Graphics::GetViewConstants();
    viewConstants.Time_Random_Luminance_X[2] = lum;
    Graphics::UpdateViewConstants(viewConstants);
}

//------------------------------------------------------------------------------
/**
*/
void
HistogramContext::UpdateConstants()
{
    float minLuminance = Core::CVarReadFloat(histogramState.minLuminance);
    Core::CVarSetModified(histogramState.minLuminance, false);

    histogramState.logMinLuminance = Math::log2(minLuminance);
    HistogramCs::HistogramConstants constants;
    constants.Mip = histogramState.mip;
    constants.WindowOffset[0] = histogramState.offset.x;
    constants.WindowOffset[1] = histogramState.offset.y;
    constants.TextureSize[0] = histogramState.size.x;
    constants.TextureSize[1] = histogramState.size.y;
    constants.InvLogLuminanceRange = 1 / histogramState.logLuminanceRange;
    constants.MinLogLuminance = histogramState.logMinLuminance;
    CoreGraphics::BufferUpdate(histogramState.histogramConstants, constants, 0);
    CoreGraphics::BufferFlush(histogramState.histogramConstants);
}

//------------------------------------------------------------------------------
/**
*/
void
HistogramContext::WindowResized(const CoreGraphics::WindowId windowId, SizeT width, SizeT height)
{
    histogramState.sourceTextureDimensions = CoreGraphics::TextureGetDimensions(histogramState.sourceTexture);

    // setup views for output and bind
    SizeT numMips = CoreGraphics::TextureGetNumMips(histogramState.sourceTexture);
    for (IndexT i = 0; i < numMips; i++)
    {
        CoreGraphics::TextureViewReload(histogramState.downsampledColorBufferViews[i]);
        if (i == 5)
        {
            CoreGraphics::ResourceTableSetRWTexture(histogramState.downsampleResourceTable,
            {
                histogramState.downsampledColorBufferViews[i],
                CoreGraphics::ShaderGetResourceSlot(histogramState.downsampleShader, "Output6"),
                0,
                CoreGraphics::InvalidSamplerId,
                false,
                false
            });
        }
        else
        {
            CoreGraphics::ResourceTableSetRWTexture(histogramState.downsampleResourceTable,
            {
                histogramState.downsampledColorBufferViews[i],
                CoreGraphics::ShaderGetResourceSlot(histogramState.downsampleShader, "Output"),
                i,
                CoreGraphics::InvalidSamplerId,
                false,
                false
            });
        }
    }
    CoreGraphics::ResourceTableCommitChanges(histogramState.downsampleResourceTable);

    CoreGraphics::ResourceTableSetTexture(histogramState.histogramResourceTable,
    {
        histogramState.sourceTexture,
        histogramState.sourceTextureBinding,
        0,
        CoreGraphics::InvalidSamplerId,
        false,
        false
    });
    CoreGraphics::ResourceTableCommitChanges(histogramState.histogramResourceTable);
}

} // namespace PostEffects
