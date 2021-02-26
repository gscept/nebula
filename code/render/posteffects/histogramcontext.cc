//------------------------------------------------------------------------------
//  histogramcontext.cc
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "frame/framescript.h"
#include "graphics/graphicsserver.h"
#include "histogramcontext.h"
#include "frame/frameplugin.h"

#include "downscale_cs.h"
#include "histogram_cs.h"
namespace PostEffects
{

__ImplementPluginContext(PostEffects::HistogramContext);
struct
{

	CoreGraphics::ShaderId histogramShader;
	CoreGraphics::ShaderProgramId histogramCategorizeProgram;
	CoreGraphics::BufferId histogramCounters;
	CoreGraphics::BufferId histogramConstants;
	CoreGraphics::ResourceTableId histogramResourceTable;

	CoreGraphics::ShaderId downsampleShader;
	CoreGraphics::ShaderProgramId downsampleProgram;
	CoreGraphics::BufferId downsampleCounter;
	CoreGraphics::BufferId downsampleConstants;
	CoreGraphics::ResourceTableId downsampleResourceTable;

	CoreGraphics::TextureDimensions sourceTextureDimensions;
	Util::FixedArray<CoreGraphics::TextureViewId> downsampledColorBufferViews;

	Math::float2 offset, size;

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
	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

	histogramState.histogramShader = CoreGraphics::ShaderGet("shd:histogram_cs.fxb");
	histogramState.histogramCategorizeProgram = CoreGraphics::ShaderGetProgram(histogramState.histogramShader, CoreGraphics::ShaderFeatureFromString("HistogramCategorize"));

	CoreGraphics::BufferCreateInfo bufInfo;
	bufInfo.elementSize = sizeof(HistogramCs::ColorValueCounters);
	bufInfo.size = 1;
	bufInfo.usageFlags = CoreGraphics::ReadWriteBuffer;
	bufInfo.mode = CoreGraphics::DeviceLocal;
	bufInfo.queueSupport = CoreGraphics::GraphicsQueueSupport;
	uint initDatas[255] = { 0 };
	bufInfo.data = initDatas;
	bufInfo.dataSize = sizeof(initDatas);
	histogramState.histogramCounters = CoreGraphics::CreateBuffer(bufInfo);

	bufInfo.elementSize = sizeof(HistogramCs::HistogramConstants);
	bufInfo.mode = CoreGraphics::HostToDevice; // lazy but meh
	bufInfo.usageFlags = CoreGraphics::ConstantBuffer;
	bufInfo.data = nullptr;
	bufInfo.dataSize = 0;
	histogramState.histogramConstants = CoreGraphics::CreateBuffer(bufInfo);

	histogramState.histogramResourceTable = CoreGraphics::ShaderCreateResourceTable(histogramState.histogramShader, NEBULA_BATCH_GROUP);
	CoreGraphics::ResourceTableSetRWBuffer(histogramState.histogramResourceTable, {
		histogramState.histogramCounters,
		CoreGraphics::ShaderGetResourceSlot(histogramState.histogramShader, "ColorValueCounters"),
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


	histogramState.downsampleShader = CoreGraphics::ShaderGet("shd:downsample/downsample_cs.fxb");
	histogramState.downsampleProgram = CoreGraphics::ShaderGetProgram(histogramState.downsampleShader, CoreGraphics::ShaderFeatureFromString("DownscaleMin"));

	// create counter for downsample shader
	bufInfo.elementSize = sizeof(uint);
	bufInfo.size = 1;
	bufInfo.usageFlags = CoreGraphics::ReadWriteBuffer;
	bufInfo.mode = CoreGraphics::DeviceLocal;
	bufInfo.queueSupport = CoreGraphics::GraphicsQueueSupport;
	uint initData = 0;
	bufInfo.data = &initData;
	bufInfo.dataSize = sizeof(initData);
	histogramState.downsampleCounter = CoreGraphics::CreateBuffer(bufInfo);

	bufInfo.elementSize = sizeof(DownscaleCs::DownscaleUniforms);
	bufInfo.mode = CoreGraphics::HostToDevice;
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
		CoreGraphics::ShaderGetResourceSlot(histogramState.downsampleShader, "DownscaleUniforms"),
		0,
		false,
		false,
		CoreGraphics::BufferGetByteSize(histogramState.downsampleConstants),
		0
	});
	CoreGraphics::ResourceTableCommitChanges(histogramState.downsampleResourceTable);

	Frame::AddCallback("HistogramContext - Downsample", [](const IndexT frame, const IndexT bufferIndex)
	{
		CoreGraphics::CommandBufferBeginMarker(CoreGraphics::GraphicsQueueType, NEBULA_MARKER_COMPUTE, "Histogram Downsample");
		CoreGraphics::SetShaderProgram(histogramState.downsampleProgram);
		CoreGraphics::SetResourceTable(histogramState.downsampleResourceTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
		uint dispatchX = (histogramState.sourceTextureDimensions.width - 1) / 64;
		uint dispatchY = (histogramState.sourceTextureDimensions.height - 1) / 64;
		CoreGraphics::Compute(dispatchX + 1, dispatchY + 1, 1);
		CoreGraphics::CommandBufferEndMarker(CoreGraphics::GraphicsQueueType);
	});

	Frame::AddCallback("HistogramContext - Bucket", [](const IndexT frame, const IndexT bufferIndex)
	{
		//CoreGraphics::SetShaderProgram(histogramState.histogramCategorizeProgram);
		//CoreGraphics::SetResourceTable(histogramState.histogramResourceTable, NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
	});
}

//------------------------------------------------------------------------------
/**
*/
void
HistogramContext::Discard()
{
}

//------------------------------------------------------------------------------
/**
*/
void
HistogramContext::SetWindow(const Math::float2 offset, Math::float2 size, int mip)
{
	histogramState.offset = offset;
	histogramState.size = size;

	HistogramCs::HistogramConstants constants;
	constants.Mip = mip;
	constants.WindowOffset[0] = histogramState.sourceTextureDimensions.width * offset.x;
	constants.WindowOffset[1] = histogramState.sourceTextureDimensions.height * offset.y;
	constants.TextureSize[0] = histogramState.sourceTextureDimensions.width * size.x;
	constants.TextureSize[1] = histogramState.sourceTextureDimensions.height * size.y;
	CoreGraphics::BufferUpdate(histogramState.histogramConstants, constants, 0);
}

//------------------------------------------------------------------------------
/**
*/
void
HistogramContext::Setup(const Ptr<Frame::FrameScript>& script)
{
	CoreGraphics::TextureId colorBuffer = script->GetTexture("LightBuffer");
	CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(colorBuffer);

	// update table with source texture
	CoreGraphics::ResourceTableSetRWTexture(histogramState.downsampleResourceTable, {
		colorBuffer,
		CoreGraphics::ShaderGetResourceSlot(histogramState.downsampleShader, "Source"),
		0,
		CoreGraphics::InvalidSamplerId,
		false,
		false
	});

	// setup views for output and bind
	SizeT numMips = CoreGraphics::TextureGetNumMips(colorBuffer);
	histogramState.downsampledColorBufferViews.Resize(numMips);
	for (IndexT i = 0; i < numMips; i++)
	{
		CoreGraphics::TextureViewCreateInfo info;
		info.tex = colorBuffer;
		info.startMip = i;
		info.numMips = 1;
		info.startLayer = 0;
		info.numLayers = 1;
		info.format = CoreGraphics::TextureGetPixelFormat(colorBuffer);
		histogramState.downsampledColorBufferViews[i] = CoreGraphics::CreateTextureView(info);

		CoreGraphics::ResourceTableSetRWTexture(histogramState.downsampleResourceTable, {
			histogramState.downsampledColorBufferViews[i],
			CoreGraphics::ShaderGetResourceSlot(histogramState.downsampleShader, "Output"),
			i,
			CoreGraphics::InvalidSamplerId,
			false,
			false
		});
	}
	CoreGraphics::ResourceTableCommitChanges(histogramState.downsampleResourceTable);

	uint dispatchX = (dims.width - 1) / 64;
	uint dispatchY = (dims.height - 1) / 64;

	DownscaleCs::DownscaleUniforms constants;
	constants.Mips = numMips;
	constants.NumGroups = (dispatchX + 1) * (dispatchY + 1);
	BufferUpdate(histogramState.downsampleConstants, constants, 0);

	histogramState.sourceTextureDimensions = dims;
	histogramState.offset.x = 0;
	histogramState.offset.y = 0;
	histogramState.size.x = 1.0f;
	histogramState.size.y = 1.0f;
}

//------------------------------------------------------------------------------
/**
*/
void
HistogramContext::UpdateViewResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{

}

} // namespace PostEffects
