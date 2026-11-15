//------------------------------------------------------------------------------
//  dofcontext.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "frame/framesubgraph.h"
#include "frame/framecode.h"
#include "coregraphics/resourcetable.h"
#include "coregraphics/barrier.h"
#include "coregraphics/texture.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "graphics/cameracontext.h"
#include "dofcontext.h"

#include "gpulang/render/system_shaders/dof.h"

#include "frame/default.h"
namespace PostEffects
{

__ImplementPluginContext(PostEffects::DofContext);
struct
{
    CoreGraphics::ShaderProgramId maskProgram;
    CoreGraphics::ShaderProgramId setupProgram;
    CoreGraphics::ShaderProgramId gatherProgram;
    CoreGraphics::ShaderProgramId compositeProgram;
    CoreGraphics::ShaderId shader;

    CoreGraphics::ResourceTableId resourceTable;
    CoreGraphics::BufferId constants;
    CoreGraphics::TextureId bokehMask;
    CoreGraphics::TextureId dirtMask;
    bool bokehDirty;

    Dof::DofUniforms::STRUCT uniforms;

} dofState;

//------------------------------------------------------------------------------
/**
    ColorSize is LightBuffer and HalfColor, the full-resolution circle of confusion.
    HalfSize is BokehBuffer, the half-resolution gather.
*/
static void
WriteDofSizes()
{
    CoreGraphics::TextureDimensions colorDims = CoreGraphics::TextureGetDimensions(FrameScript_default::Texture_LightBuffer());
    CoreGraphics::TextureDimensions halfColorDims = CoreGraphics::TextureGetDimensions(FrameScript_default::Texture_HalfColor());
    CoreGraphics::TextureDimensions bokehDims = CoreGraphics::TextureGetDimensions(FrameScript_default::Texture_BokehBuffer());
    n_assert(colorDims.width > 0);
    n_assert(colorDims.height > 0);
    n_assert(halfColorDims.width >= colorDims.width);
    n_assert(halfColorDims.height >= colorDims.height);
    n_assert(bokehDims.width >= (colorDims.width >> 1));
    n_assert(bokehDims.height >= (colorDims.height >> 1));

    dofState.uniforms.ColorSize[0] = int32_t(colorDims.width);
    dofState.uniforms.ColorSize[1] = int32_t(colorDims.height);
    dofState.uniforms.ColorSize[2] = 0;
    dofState.uniforms.ColorSize[3] = 0;
    dofState.uniforms.HalfSize[0] = int32_t(bokehDims.width);
    dofState.uniforms.HalfSize[1] = int32_t(bokehDims.height);
    dofState.uniforms.HalfSize[2] = 0;
    dofState.uniforms.HalfSize[3] = 0;
}

//------------------------------------------------------------------------------
/**
    ColorSource is LightBuffer. BokehOutput is BokehBuffer.
*/
static void
BindDofResources()
{
    using namespace CoreGraphics;
    ResourceTableSetTexture(dofState.resourceTable, { FrameScript_default::Texture_LightBuffer(), Dof::ColorSource::BINDING });
    ResourceTableSetRWTexture(dofState.resourceTable, { FrameScript_default::Texture_LightBuffer(), Dof::ColorTarget::BINDING });
    ResourceTableSetRWTexture(dofState.resourceTable, { FrameScript_default::Texture_HalfColor(), Dof::HalfColor::BINDING });
    ResourceTableSetRWTexture(dofState.resourceTable, { FrameScript_default::Texture_BokehBuffer(), Dof::BokehOutput::BINDING });
    ResourceTableSetTexture(dofState.resourceTable, { FrameScript_default::Texture_BokehBuffer(), Dof::BokehInput::BINDING });
    ResourceTableSetTexture(dofState.resourceTable, { dofState.dirtMask, Dof::DirtMask::BINDING });
    ResourceTableSetTexture(dofState.resourceTable, { dofState.bokehMask, Dof::BokehMask::BINDING });
    ResourceTableSetRWTexture(dofState.resourceTable, { dofState.bokehMask, Dof::BokehMaskWrite::BINDING });
    ResourceTableSetConstantBuffer(dofState.resourceTable, { dofState.constants, Dof::DofUniforms::BINDING });
    ResourceTableCommitChanges(dofState.resourceTable);
}

//------------------------------------------------------------------------------
/**
*/
DofContext::DofContext()
{
}

//------------------------------------------------------------------------------
/**
*/
DofContext::~DofContext()
{
}

//------------------------------------------------------------------------------
/**
*/
void
DofContext::Setup()
{
    using namespace CoreGraphics;

    dofState.shader = ShaderGet("shd:system_shaders/dof.gplb");
    dofState.maskProgram = ShaderGetProgram(dofState.shader, ShaderFeatureMask("Mask"));
    dofState.setupProgram = ShaderGetProgram(dofState.shader, ShaderFeatureMask("Setup"));
    dofState.gatherProgram = ShaderGetProgram(dofState.shader, ShaderFeatureMask("Gather"));
    dofState.compositeProgram = ShaderGetProgram(dofState.shader, ShaderFeatureMask("Composite"));
    dofState.resourceTable = ShaderCreateResourceTable(dofState.shader, NEBULA_BATCH_GROUP);

    BufferCreateInfo bufInfo;
    bufInfo.byteSize = sizeof(Dof::DofUniforms::STRUCT);
    bufInfo.usageFlags = BufferUsage::ConstantBuffer | BufferUsage::TransferDestination;
    bufInfo.mode = DeviceAndHost;
    bufInfo.queueSupport = GraphicsQueueSupport | ComputeQueueSupport;
    dofState.constants = CreateBuffer(bufInfo);

    // Aperture.xyzw: blade count, rotation in radians, roundness, unused.
    // A blade count outside 3..8 is a disk. The first DofMask writes that disk.
    Memory::Clear(&dofState.uniforms, sizeof(dofState.uniforms));
    if (dofState.dirtMask == CoreGraphics::InvalidTextureId)
        dofState.dirtMask = CoreGraphics::White2D;
    // Aperture.w is 0 for the white placeholder. Gather then weights every aperture sample by 1.
    dofState.uniforms.Aperture[3] = dofState.dirtMask == CoreGraphics::White2D ? 0.0f : 1.0f;
    dofState.bokehDirty = true;
    WriteDofSizes();
    BufferUpdate(dofState.constants, dofState.uniforms);

    TextureCreateInfo maskInfo;
    maskInfo.name = "BokehMask";
    maskInfo.format = PixelFormat::R16G16B16A16F;
    maskInfo.width = 31;
    maskInfo.height = 1;
    maskInfo.usage = TextureUsage::ReadWrite;
    dofState.bokehMask = CreateTexture(maskInfo);

    BindDofResources();

    // DofMask. 31 threads, one per mask row. Compute Prepass, before Scene.
    FrameScript_default::RegisterSubgraph_DoFMask_Compute([](const CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, dofState.maskProgram, queue);
        CmdSetResourceTable(cmdBuf, dofState.resourceTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);

        // DofUniforms.Aperture from the view camera. SetBokeh rebuilds the mask when it changes.
        const Graphics::ViewId view = Graphics::GraphicsServer::Instance()->GetCurrentView();
        n_assert(view != Graphics::InvalidViewId);
        const Graphics::DepthOfField& dof = Graphics::CameraContext::GetDepthOfField(Graphics::ViewGetCamera(view));
        SetBokeh(dof.blades, dof.rotation, dof.roundness);
        CoreGraphics::TextureId dirt = CoreGraphics::White2D;
        if (dof.dirtMask != Resources::InvalidResourceId)
            dirt = CoreGraphics::TextureId(dof.dirtMask);
        SetDirtMask(dirt);

        WriteDofSizes();
        CmdUpdateBuffer(cmdBuf, dofState.constants, 0, sizeof(dofState.uniforms), &dofState.uniforms);

        // Shader-read outside this store. General only for the write, then back.
        // This dispatch is on the compute queue, so the read stage is ComputeShaderRead.
        if (dofState.bokehDirty)
        {
            CmdBarrier(cmdBuf, PipelineStage::ComputeShaderRead, PipelineStage::ComputeShaderWrite, BarrierDomain::Global,
            {
                TextureBarrierInfo{ dofState.bokehMask, TextureSubresourceInfo::ColorNoMipNoLayer() }
            });
            CmdDispatch(cmdBuf, 1, 1, 1);
            CmdBarrier(cmdBuf, PipelineStage::ComputeShaderWrite, PipelineStage::ComputeShaderRead, BarrierDomain::Global,
            {
                TextureBarrierInfo{ dofState.bokehMask, TextureSubresourceInfo::ColorNoMipNoLayer() }
            });
            dofState.bokehDirty = false;
        }
    }, nullptr, nullptr);

    // DofSetup. Full-res tile grid. Reads LightBuffer and ZBuffer, writes HalfColor.
    FrameScript_default::RegisterSubgraph_DoFSetup_Compute([](const CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, dofState.setupProgram, queue);
        CmdSetResourceTable(cmdBuf, dofState.resourceTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);

        uint dispatchX = Math::divandroundup(uint(viewport.width()), 16);
        uint dispatchY = Math::divandroundup(uint(viewport.height()), 16);
        CmdDispatch(cmdBuf, dispatchX, dispatchY, 1);
    }, nullptr, {
        { FrameScript_default::TextureIndex::LightBuffer, PipelineStage::ComputeShaderRead }
        , { FrameScript_default::TextureIndex::ZBuffer, PipelineStage::ComputeShaderRead }
        , { FrameScript_default::TextureIndex::HalfColor, PipelineStage::ComputeShaderWrite }
    });

    // DofGather. Half-res tile grid. Reads full-res HalfColor, writes BokehBuffer.
    FrameScript_default::RegisterSubgraph_DoFGather_Compute([](const CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, dofState.gatherProgram, queue);
        CmdSetResourceTable(cmdBuf, dofState.resourceTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);

        // BokehMask is already shader-read. Acquire the store before the fetch.
        CmdBarrier(cmdBuf, PipelineStage::AllShadersRead, PipelineStage::ComputeShaderRead, BarrierDomain::Global,
        {
            TextureBarrierInfo{ dofState.bokehMask, TextureSubresourceInfo::ColorNoMipNoLayer() }
        });

        uint dispatchX = Math::divandroundup(uint(viewport.width()) >> 1, 16);
        uint dispatchY = Math::divandroundup(uint(viewport.height()) >> 1, 16);
        CmdDispatch(cmdBuf, dispatchX, dispatchY, 1);
    }, nullptr, {
        { FrameScript_default::TextureIndex::HalfColor, PipelineStage::ComputeShaderRead }
        , { FrameScript_default::TextureIndex::BokehBuffer, PipelineStage::ComputeShaderWrite }
    });

    // DofComposite. Writes the depth-of-field mix into LightBuffer before Color Downsample.
    FrameScript_default::RegisterSubgraph_DoFComposite_Compute([](const CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, dofState.compositeProgram, queue);
        CmdSetResourceTable(cmdBuf, dofState.resourceTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);

        uint dispatchX = Math::divandroundup(uint(viewport.width()), 16);
        uint dispatchY = Math::divandroundup(uint(viewport.height()), 16);
        CmdDispatch(cmdBuf, dispatchX, dispatchY, 1);
    }, nullptr, {
        { FrameScript_default::TextureIndex::LightBuffer, PipelineStage::ComputeShaderWrite }
        , { FrameScript_default::TextureIndex::BokehBuffer, PipelineStage::ComputeShaderRead }
        , { FrameScript_default::TextureIndex::ZBuffer, PipelineStage::ComputeShaderRead }
    });
}

//------------------------------------------------------------------------------
/**
*/
void
DofContext::SetBokeh(float blades, float rotation, float roundness)
{
    n_assert(roundness >= 0.0f && roundness <= 1.0f);
    if (dofState.uniforms.Aperture[0] == blades
        && dofState.uniforms.Aperture[1] == rotation
        && dofState.uniforms.Aperture[2] == roundness)
        return;

    dofState.uniforms.Aperture[0] = blades;
    dofState.uniforms.Aperture[1] = rotation;
    dofState.uniforms.Aperture[2] = roundness;
    dofState.bokehDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
void
DofContext::SetDirtMask(const CoreGraphics::TextureId texture)
{
    n_assert(texture != CoreGraphics::InvalidTextureId);
    if (dofState.dirtMask == texture)
        return;
    dofState.dirtMask = texture;
    dofState.uniforms.Aperture[3] = texture == CoreGraphics::White2D ? 0.0f : 1.0f;
    if (dofState.resourceTable != CoreGraphics::InvalidResourceTableId)
    {
        CoreGraphics::ResourceTableSetTexture(dofState.resourceTable, { texture, Dof::DirtMask::BINDING });
        CoreGraphics::ResourceTableCommitChanges(dofState.resourceTable);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DofContext::Resize(const uint framescriptHash, SizeT width, SizeT height)
{
    if (framescriptHash == FrameScript_default::ID)
    {
        BindDofResources();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DofContext::Create()
{
    __CreatePluginContext();
    __bundle.OnViewportResized = DofContext::Resize;
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);
}

//------------------------------------------------------------------------------
/**
*/
void
DofContext::Discard()
{
    CoreGraphics::DestroyResourceTable(dofState.resourceTable);
    CoreGraphics::DestroyBuffer(dofState.constants);
    CoreGraphics::DestroyTexture(dofState.bokehMask);
}

} // namespace PostEffects
