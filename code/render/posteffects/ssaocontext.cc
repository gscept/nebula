//------------------------------------------------------------------------------
//  ssaocontext.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "frame/frameplugin.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/resourcetable.h"
#include "coregraphics/sampler.h"
#include "coregraphics/shadersemantics.h"
#include "graphics/cameracontext.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "ssaocontext.h"

#include "hbao_cs.h"
#include "hbaoblur_cs.h"

namespace PostEffects
{
__ImplementPluginContext(PostEffects::SSAOContext);

struct
{
    CoreGraphics::ShaderId hbaoShader, blurShader;
    CoreGraphics::ShaderProgramId xDirectionHBAO, yDirectionHBAO, xDirectionBlur, yDirectionBlur;

    Util::FixedArray<CoreGraphics::ResourceTableId> hbaoTable, blurTableX, blurTableY;
    //CoreGraphics::ResourceTableId hbaoTable, blurTableX, blurTableY;
    CoreGraphics::BufferId hbaoConstants, blurConstants;
    IndexT hbao0, hbao1, hbaoX, hbaoY, hbaoBlurRG, hbaoBlurR, hbaoC, blurC;

    IndexT uvToViewAVar, uvToViewBVar, r2Var,
        aoResolutionVar, invAOResolutionVar, strengthVar, tanAngleBiasVar,
        powerExponentVar, blurFalloff, blurDepthThreshold;

    // read-write textures
    CoreGraphics::TextureId internalTargets[2];
    CoreGraphics::TextureId ssaoOutput;

    CoreGraphics::BarrierId barriers[4];

    struct AOVariables
    {
        float fullWidth, fullHeight;
        float width, height;
        float downsample;
        float nearZ, farZ;
        float sceneScale;
        Math::vec2 uvToViewA;
        Math::vec2 uvToViewB;
        float radius, r, r2;
        float negInvR2;
        Math::vec2 focalLength;
        Math::vec2 aoResolution;
        Math::vec2 invAOResolution;
        float maxRadiusPixels;
        float strength;
        float tanAngleBias;
        float blurThreshold;
        float blurFalloff;
    } vars;
} ssaoState;

//------------------------------------------------------------------------------
/**
*/
SSAOContext::SSAOContext()
{
}

//------------------------------------------------------------------------------
/**
*/
SSAOContext::~SSAOContext()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
SSAOContext::Create()
{
    __CreatePluginContext();

    __bundle.OnUpdateViewResources = SSAOContext::UpdateViewDependentResources;
    __bundle.OnWindowResized = SSAOContext::WindowResized;
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    // calculate HBAO and blur
    using namespace CoreGraphics;
    Frame::AddCallback("HBAO-Run", [](const IndexT frame, const IndexT bufferIndex)
        {
            ShaderServer* shaderServer = ShaderServer::Instance();

            // get final dimensions
            SizeT width = ssaoState.vars.width;
            SizeT height = ssaoState.vars.height;

            // calculate execution dimensions
            uint numGroupsX1 = Math::divandroundup(width, HbaoCs::HBAOTileWidth);
            uint numGroupsX2 = width;
            uint numGroupsY1 = Math::divandroundup(height, HbaoCs::HBAOTileWidth);
            uint numGroupsY2 = height;

            // we are running the SSAO on the graphics queue
#if NEBULA_GRAPHICS_DEBUG
            CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "HBAO");
#endif

            // render AO in X
            CoreGraphics::SetShaderProgram(ssaoState.xDirectionHBAO);
            CoreGraphics::SetResourceTable(ssaoState.hbaoTable[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
            CoreGraphics::BarrierInsert(ssaoState.barriers[0], GraphicsQueueType); // transition from shader read to general
            CoreGraphics::Compute(numGroupsX1, numGroupsY2, 1);

            // now do it in Y
            CoreGraphics::SetShaderProgram(ssaoState.yDirectionHBAO);
            CoreGraphics::SetResourceTable(ssaoState.hbaoTable[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
            CoreGraphics::BarrierInsert(ssaoState.barriers[1], GraphicsQueueType); // transition from shader read to general
            CoreGraphics::Compute(numGroupsY1, numGroupsX2, 1);

            // blur in X
            CoreGraphics::SetShaderProgram(ssaoState.xDirectionBlur, CoreGraphics::GraphicsQueueType, false);
            CoreGraphics::SetResourceTable(ssaoState.blurTableX[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
            CoreGraphics::BarrierInsert(ssaoState.barriers[2], GraphicsQueueType); // transition from shader read to general
            CoreGraphics::Compute(numGroupsX1, numGroupsY2, 1);

            // blur in Y
            CoreGraphics::SetShaderProgram(ssaoState.yDirectionBlur, CoreGraphics::GraphicsQueueType, false);
            CoreGraphics::SetResourceTable(ssaoState.blurTableY[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
            CoreGraphics::BarrierInsert(ssaoState.barriers[3], GraphicsQueueType);
            CoreGraphics::Compute(numGroupsY1, numGroupsX2, 1);

#if NEBULA_GRAPHICS_DEBUG
            CoreGraphics::CommandBufferEndMarker(GraphicsQueueType);
#endif
        });
}

//------------------------------------------------------------------------------
/**
*/
void 
SSAOContext::Discard()
{
    CoreGraphics::DestroyTexture(ssaoState.internalTargets[0]);
    CoreGraphics::DestroyTexture(ssaoState.internalTargets[1]);
    IndexT i;
    for (i = 0; i < ssaoState.hbaoTable.Size(); i++)
    {
        DestroyResourceTable(ssaoState.hbaoTable[i]);
        DestroyResourceTable(ssaoState.blurTableX[i]);
        DestroyResourceTable(ssaoState.blurTableY[i]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
SSAOContext::Setup(const Ptr<Frame::FrameScript>& script)
{
    using namespace CoreGraphics;
    CoreGraphics::TextureCreateInfo tinfo;
    tinfo.name = "HBAO-Internal0"_atm;
    tinfo.tag = "system"_atm;
    tinfo.type = Texture2D;
    tinfo.format = PixelFormat::R16G16F;
    tinfo.windowRelative = true;
    tinfo.usage = TextureUsage::ReadWriteTexture;

    ssaoState.internalTargets[0] = CreateTexture(tinfo);
    tinfo.name = "HBAO-Internal1";
    ssaoState.internalTargets[1] = CreateTexture(tinfo);

    CoreGraphics::BarrierCreateInfo binfo =
    {
        ""_atm,
        BarrierDomain::Global,
        BarrierStage::ComputeShader,
        BarrierStage::ComputeShader
    };
    ImageSubresourceInfo subres = ImageSubresourceInfo::ColorNoMipNoLayer();

    // hbao generation barriers
    binfo.name = "HBAO Initial transition";
    binfo.textures.Append(TextureBarrier{ ssaoState.internalTargets[0], subres, CoreGraphics::ImageLayout::ShaderRead, CoreGraphics::ImageLayout::General, BarrierAccess::ShaderRead, BarrierAccess::ShaderWrite });
    binfo.textures.Append(TextureBarrier{ ssaoState.internalTargets[1], subres, CoreGraphics::ImageLayout::ShaderRead, CoreGraphics::ImageLayout::General, BarrierAccess::ShaderRead, BarrierAccess::ShaderWrite });
    ssaoState.barriers[0] = CreateBarrier(binfo);
    binfo.textures.Clear();

    binfo.name = "HBAO Transition Pass 0 -> 1";
    binfo.textures.Append(TextureBarrier{ ssaoState.internalTargets[0], subres, CoreGraphics::ImageLayout::General, CoreGraphics::ImageLayout::General, BarrierAccess::ShaderWrite, BarrierAccess::ShaderRead });
    binfo.textures.Append(TextureBarrier{ ssaoState.internalTargets[1], subres, CoreGraphics::ImageLayout::General, CoreGraphics::ImageLayout::General, BarrierAccess::ShaderRead, BarrierAccess::ShaderWrite });
    ssaoState.barriers[1] = CreateBarrier(binfo);
    binfo.textures.Clear();

    // hbao blur barriers
    binfo.name = "HBAO Transition to Blur";
    binfo.textures.Append(TextureBarrier{ ssaoState.internalTargets[1], subres, CoreGraphics::ImageLayout::General, CoreGraphics::ImageLayout::ShaderRead, BarrierAccess::ShaderWrite, BarrierAccess::ShaderRead });
    binfo.textures.Append(TextureBarrier{ ssaoState.internalTargets[0], subres, CoreGraphics::ImageLayout::General, CoreGraphics::ImageLayout::General, BarrierAccess::ShaderRead, BarrierAccess::ShaderWrite });
    ssaoState.barriers[2] = CreateBarrier(binfo);
    binfo.textures.Clear();

    binfo.name = "HBAO Transition to Blur Pass 0 -> 1";
    binfo.textures.Append(TextureBarrier{ ssaoState.internalTargets[0], subres, CoreGraphics::ImageLayout::General, CoreGraphics::ImageLayout::ShaderRead, BarrierAccess::ShaderWrite, BarrierAccess::ShaderRead });
    binfo.textures.Append(TextureBarrier{ ssaoState.internalTargets[1], subres, CoreGraphics::ImageLayout::ShaderRead, CoreGraphics::ImageLayout::ShaderRead, BarrierAccess::ShaderRead, BarrierAccess::ShaderWrite });
    ssaoState.barriers[3] = CreateBarrier(binfo);
    binfo.textures.Clear();

    // setup shaders
    ssaoState.hbaoShader = ShaderGet("shd:hbao_cs.fxb");
    ssaoState.blurShader = ShaderGet("shd:hbaoblur_cs.fxb");
    ssaoState.hbao0 = ShaderGetResourceSlot(ssaoState.hbaoShader, "HBAO0");
    ssaoState.hbao1 = ShaderGetResourceSlot(ssaoState.hbaoShader, "HBAO1");
    ssaoState.hbaoC = ShaderGetResourceSlot(ssaoState.hbaoShader, "HBAOBlock");
    ssaoState.hbaoX = ShaderGetResourceSlot(ssaoState.blurShader, "HBAOX");
    ssaoState.hbaoY = ShaderGetResourceSlot(ssaoState.blurShader, "HBAOY");
    ssaoState.hbaoBlurRG = ShaderGetResourceSlot(ssaoState.blurShader, "HBAORG");
    ssaoState.hbaoBlurR = ShaderGetResourceSlot(ssaoState.blurShader, "HBAOR");
    ssaoState.blurC = ShaderGetResourceSlot(ssaoState.blurShader, "HBAOBlur");

    ssaoState.xDirectionHBAO = ShaderGetProgram(ssaoState.hbaoShader, ShaderFeatureFromString("Alt0"));
    ssaoState.yDirectionHBAO = ShaderGetProgram(ssaoState.hbaoShader, ShaderFeatureFromString("Alt1"));
    ssaoState.xDirectionBlur = ShaderGetProgram(ssaoState.blurShader, ShaderFeatureFromString("Alt0"));
    ssaoState.yDirectionBlur = ShaderGetProgram(ssaoState.blurShader, ShaderFeatureFromString("Alt1"));

    ssaoState.hbaoConstants = CoreGraphics::GetComputeConstantBuffer(MainThreadConstantBuffer);
    ssaoState.blurConstants = CoreGraphics::GetComputeConstantBuffer(MainThreadConstantBuffer);

    ssaoState.ssaoOutput = script->GetTexture("SSAOBuffer");
    SizeT numBuffers = CoreGraphics::GetNumBufferedFrames();
    ssaoState.hbaoTable.Resize(numBuffers);
    ssaoState.blurTableX.Resize(numBuffers);
    ssaoState.blurTableY.Resize(numBuffers);
    IndexT i;
    for (i = 0; i < numBuffers; i++)
    {
        ssaoState.hbaoTable[i] = ShaderCreateResourceTable(ssaoState.hbaoShader, NEBULA_BATCH_GROUP, numBuffers);
        ssaoState.blurTableX[i] = ShaderCreateResourceTable(ssaoState.blurShader, NEBULA_BATCH_GROUP, numBuffers);
        ssaoState.blurTableY[i] = ShaderCreateResourceTable(ssaoState.blurShader, NEBULA_BATCH_GROUP, numBuffers);

        // setup hbao table
        ResourceTableSetRWTexture(ssaoState.hbaoTable[i], { ssaoState.internalTargets[0], ssaoState.hbao0, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetRWTexture(ssaoState.hbaoTable[i], { ssaoState.internalTargets[1], ssaoState.hbao1, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableCommitChanges(ssaoState.hbaoTable[i]);

        // setup blur table
        ResourceTableSetTexture(ssaoState.blurTableX[i], { ssaoState.internalTargets[1], ssaoState.hbaoX, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetRWTexture(ssaoState.blurTableX[i], { ssaoState.internalTargets[0], ssaoState.hbaoBlurRG, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableCommitChanges(ssaoState.blurTableX[i]);

        ResourceTableSetTexture(ssaoState.blurTableY[i], { ssaoState.internalTargets[0], ssaoState.hbaoY, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetRWTexture(ssaoState.blurTableY[i], { ssaoState.ssaoOutput, ssaoState.hbaoBlurR, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableCommitChanges(ssaoState.blurTableY[i]);
    }

    TextureDimensions dims = TextureGetDimensions(ssaoState.ssaoOutput);
    ssaoState.vars.fullWidth = (float)dims.width;
    ssaoState.vars.fullHeight = (float)dims.height;
    ssaoState.vars.radius = 12.0f;
    ssaoState.vars.downsample = 1.0f;
    ssaoState.vars.sceneScale = 1.0f;

#define MAX_RADIUS_PIXELS 0.5f
    ssaoState.vars.maxRadiusPixels = MAX_RADIUS_PIXELS * Math::min(ssaoState.vars.fullWidth, ssaoState.vars.fullHeight);
    ssaoState.vars.tanAngleBias = tanf(Math::deg2rad(35.0));
    ssaoState.vars.strength = 1.0f;

    // setup hbao params
    ssaoState.uvToViewAVar = ShaderGetConstantBinding(ssaoState.hbaoShader, NEBULA_SEMANTIC_UVTOVIEWA);
    ssaoState.uvToViewBVar = ShaderGetConstantBinding(ssaoState.hbaoShader, NEBULA_SEMANTIC_UVTOVIEWB);
    ssaoState.r2Var = ShaderGetConstantBinding(ssaoState.hbaoShader, NEBULA_SEMANTIC_R2);
    ssaoState.aoResolutionVar = ShaderGetConstantBinding(ssaoState.hbaoShader, NEBULA_SEMANTIC_AORESOLUTION);
    ssaoState.invAOResolutionVar = ShaderGetConstantBinding(ssaoState.hbaoShader, NEBULA_SEMANTIC_INVAORESOLUTION);
    ssaoState.strengthVar = ShaderGetConstantBinding(ssaoState.hbaoShader, NEBULA_SEMANTIC_STRENGHT);
    ssaoState.tanAngleBiasVar = ShaderGetConstantBinding(ssaoState.hbaoShader, NEBULA_SEMANTIC_TANANGLEBIAS);

    // setup blur params
    ssaoState.powerExponentVar = ShaderGetConstantBinding(ssaoState.blurShader, NEBULA_SEMANTIC_POWEREXPONENT);
    ssaoState.blurFalloff = ShaderGetConstantBinding(ssaoState.blurShader, NEBULA_SEMANTIC_FALLOFF);
    ssaoState.blurDepthThreshold = ShaderGetConstantBinding(ssaoState.blurShader, NEBULA_SEMANTIC_DEPTHTHRESHOLD);
}

//------------------------------------------------------------------------------
/**
*/
void 
SSAOContext::UpdateViewDependentResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
    // get camera settings
    using namespace Graphics;
    using namespace CoreGraphics;
    const CameraSettings& cameraSettings = CameraContext::GetSettings(view->GetCamera());

    ssaoState.vars.width = ssaoState.vars.fullWidth / ssaoState.vars.downsample;
    ssaoState.vars.height = ssaoState.vars.fullHeight / ssaoState.vars.downsample;

    ssaoState.vars.nearZ = cameraSettings.GetZNear() + 0.1f;
    ssaoState.vars.farZ = cameraSettings.GetZFar();

    ssaoState.vars.r = ssaoState.vars.radius * 4.0f / 100.0f;
    ssaoState.vars.r2 = ssaoState.vars.r * ssaoState.vars.r;
    ssaoState.vars.negInvR2 = -1.0f / ssaoState.vars.r2;

    ssaoState.vars.aoResolution.x = ssaoState.vars.width;
    ssaoState.vars.aoResolution.y = ssaoState.vars.height;
    ssaoState.vars.invAOResolution.x = 1.0f / ssaoState.vars.width;
    ssaoState.vars.invAOResolution.y = 1.0f / ssaoState.vars.height;

    float fov = cameraSettings.GetFov();
    ssaoState.vars.focalLength.x = 1.0f / tanf(fov * 0.5f) * (ssaoState.vars.fullHeight / ssaoState.vars.fullWidth);
    ssaoState.vars.focalLength.y = 1.0f / tanf(fov * 0.5f);

    Math::vec2 invFocalLength;
    invFocalLength.x = 1 / ssaoState.vars.focalLength.x;
    invFocalLength.y = 1 / ssaoState.vars.focalLength.y;

    ssaoState.vars.uvToViewA.x = 2.0f * invFocalLength.x;
    ssaoState.vars.uvToViewA.y = -2.0f * invFocalLength.y;
    ssaoState.vars.uvToViewB.x = -1.0f * invFocalLength.x;
    ssaoState.vars.uvToViewB.y = 1.0f * invFocalLength.y;

#ifndef INV_LN2
#define INV_LN2 1.44269504f
#endif

#ifndef SQRT_LN2
#define SQRT_LN2 0.832554611f
#endif

#define BLUR_RADIUS 33
#define BLUR_SHARPNESS 8.0f

    float blurSigma = (BLUR_RADIUS + 1) * 0.5f;
    ssaoState.vars.blurFalloff = INV_LN2 / (2.0f * blurSigma * blurSigma);
    ssaoState.vars.blurThreshold = 2.0f * SQRT_LN2 * (ssaoState.vars.sceneScale / BLUR_SHARPNESS);

    HbaoCs::HBAOBlock hbaoBlock;
    hbaoBlock.AOResolution[0] = ssaoState.vars.aoResolution.x;
    hbaoBlock.AOResolution[1] = ssaoState.vars.aoResolution.y;
    hbaoBlock.InvAOResolution[0] = ssaoState.vars.invAOResolution.x;
    hbaoBlock.InvAOResolution[1] = ssaoState.vars.invAOResolution.y;
    hbaoBlock.R2 = ssaoState.vars.r2;
    hbaoBlock.Strength = ssaoState.vars.strength;
    hbaoBlock.TanAngleBias = ssaoState.vars.tanAngleBias;
    hbaoBlock.UVToViewA[0] = ssaoState.vars.uvToViewA.x;
    hbaoBlock.UVToViewA[1] = ssaoState.vars.uvToViewA.y;
    hbaoBlock.UVToViewB[0] = ssaoState.vars.uvToViewB.x;
    hbaoBlock.UVToViewB[1] = ssaoState.vars.uvToViewB.y;
    uint hbaoOffset = CoreGraphics::SetComputeConstants(MainThreadConstantBuffer, hbaoBlock);

    IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

    ResourceTableSetConstantBuffer(ssaoState.hbaoTable[bufferIndex], { ssaoState.hbaoConstants, ssaoState.hbaoC, 0, false, false, sizeof(HbaoCs::HBAOBlock), (SizeT)hbaoOffset });
    ResourceTableCommitChanges(ssaoState.hbaoTable[bufferIndex]);

    HbaoblurCs::HBAOBlur blurBlock;
    blurBlock.BlurFalloff = ssaoState.vars.blurFalloff;
    blurBlock.BlurDepthThreshold = ssaoState.vars.blurThreshold;
    blurBlock.PowerExponent = 1.5f;
    uint blurOffset = CoreGraphics::SetComputeConstants(MainThreadConstantBuffer, blurBlock);

    ResourceTableSetConstantBuffer(ssaoState.blurTableX[bufferIndex], { ssaoState.blurConstants, ssaoState.blurC, 0, false, false, sizeof(HbaoblurCs::HBAOBlur), (SizeT)blurOffset });
    ResourceTableSetConstantBuffer(ssaoState.blurTableY[bufferIndex], { ssaoState.blurConstants, ssaoState.blurC, 0, false, false, sizeof(HbaoblurCs::HBAOBlur), (SizeT)blurOffset });
    ResourceTableCommitChanges(ssaoState.blurTableX[bufferIndex]);
    ResourceTableCommitChanges(ssaoState.blurTableY[bufferIndex]);
}

//------------------------------------------------------------------------------
/**
*/
void
SSAOContext::WindowResized(const CoreGraphics::WindowId id, SizeT width, SizeT height)
{
    using namespace CoreGraphics;
    DestroyTexture(ssaoState.internalTargets[0]);
    DestroyTexture(ssaoState.internalTargets[1]);

    TextureCreateInfo tinfo;
    tinfo.name = "HBAO-Internal0"_atm;
    tinfo.tag = "system"_atm;
    tinfo.type = CoreGraphics::Texture2D;
    tinfo.format = CoreGraphics::PixelFormat::R16G16F;
    tinfo.windowRelative = true;
    tinfo.usage = CoreGraphics::TextureUsage::ReadWriteTexture;

    ssaoState.internalTargets[0] = CreateTexture(tinfo);
    tinfo.name = "HBAO-Internal1";
    ssaoState.internalTargets[1] = CreateTexture(tinfo);

    IndexT i;
    for (i = 0; i < ssaoState.hbaoTable.Size(); i++)
    {
        // setup hbao table
        ResourceTableSetRWTexture(ssaoState.hbaoTable[i], { ssaoState.internalTargets[0], ssaoState.hbao0, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetRWTexture(ssaoState.hbaoTable[i], { ssaoState.internalTargets[1], ssaoState.hbao1, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableCommitChanges(ssaoState.hbaoTable[i]);

        // setup blur table
        ResourceTableSetTexture(ssaoState.blurTableX[i], { ssaoState.internalTargets[1], ssaoState.hbaoX, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetRWTexture(ssaoState.blurTableX[i], { ssaoState.internalTargets[0], ssaoState.hbaoBlurRG, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableCommitChanges(ssaoState.blurTableX[i]);

        ResourceTableSetTexture(ssaoState.blurTableY[i], { ssaoState.internalTargets[0], ssaoState.hbaoY, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetRWTexture(ssaoState.blurTableY[i], { ssaoState.ssaoOutput, ssaoState.hbaoBlurR, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableCommitChanges(ssaoState.blurTableY[i]);
    }

    TextureDimensions dims = TextureGetDimensions(ssaoState.ssaoOutput);
    ssaoState.vars.fullWidth = (float)dims.width;
    ssaoState.vars.fullHeight = (float)dims.height;
    ssaoState.vars.radius = 12.0f;
    ssaoState.vars.downsample = 1.0f;
    ssaoState.vars.sceneScale = 1.0f;

    ssaoState.vars.maxRadiusPixels = MAX_RADIUS_PIXELS * Math::min(ssaoState.vars.fullWidth, ssaoState.vars.fullHeight);

    BarrierCreateInfo binfo =
    {
        ""_atm,
        BarrierDomain::Global,
        BarrierStage::ComputeShader,
        BarrierStage::ComputeShader
    };
    ImageSubresourceInfo subres = ImageSubresourceInfo::ColorNoMipNoLayer();

    // hbao generation barriers
    binfo.name = "HBAO Initial transition";
    binfo.textures.Append(TextureBarrier{ ssaoState.internalTargets[0], subres, CoreGraphics::ImageLayout::ShaderRead, CoreGraphics::ImageLayout::General, BarrierAccess::ShaderRead, BarrierAccess::ShaderWrite });
    binfo.textures.Append(TextureBarrier{ ssaoState.internalTargets[1], subres, CoreGraphics::ImageLayout::ShaderRead, CoreGraphics::ImageLayout::General, BarrierAccess::ShaderRead, BarrierAccess::ShaderWrite });
    ssaoState.barriers[0] = CreateBarrier(binfo);
    binfo.textures.Clear();

    binfo.name = "HBAO Transition Pass 0 -> 1";
    binfo.textures.Append(TextureBarrier{ ssaoState.internalTargets[0], subres, CoreGraphics::ImageLayout::General, CoreGraphics::ImageLayout::General, BarrierAccess::ShaderWrite, BarrierAccess::ShaderRead });
    binfo.textures.Append(TextureBarrier{ ssaoState.internalTargets[1], subres, CoreGraphics::ImageLayout::General, CoreGraphics::ImageLayout::General, BarrierAccess::ShaderRead, BarrierAccess::ShaderWrite });
    ssaoState.barriers[1] = CreateBarrier(binfo);
    binfo.textures.Clear();

    // hbao blur barriers
    binfo.name = "HBAO Transition to Blur";
    binfo.textures.Append(TextureBarrier{ ssaoState.internalTargets[1], subres, CoreGraphics::ImageLayout::General, CoreGraphics::ImageLayout::ShaderRead, BarrierAccess::ShaderWrite, BarrierAccess::ShaderRead });
    binfo.textures.Append(TextureBarrier{ ssaoState.internalTargets[0], subres, CoreGraphics::ImageLayout::General, CoreGraphics::ImageLayout::General, BarrierAccess::ShaderRead, BarrierAccess::ShaderWrite });
    ssaoState.barriers[2] = CreateBarrier(binfo);
    binfo.textures.Clear();

    binfo.name = "HBAO Transition to Blur Pass 0 -> 1";
    binfo.textures.Append(TextureBarrier{ ssaoState.internalTargets[0], subres, CoreGraphics::ImageLayout::General, CoreGraphics::ImageLayout::ShaderRead, BarrierAccess::ShaderWrite, BarrierAccess::ShaderRead });
    binfo.textures.Append(TextureBarrier{ ssaoState.internalTargets[1], subres, CoreGraphics::ImageLayout::ShaderRead, CoreGraphics::ImageLayout::ShaderRead, BarrierAccess::ShaderRead, BarrierAccess::ShaderWrite });
    ssaoState.barriers[3] = CreateBarrier(binfo);
    binfo.textures.Clear();
}

} // namespace PostEffects
