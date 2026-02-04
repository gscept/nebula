//------------------------------------------------------------------------------
//  ssaocontext.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "frame/framesubgraph.h"
#include "frame/framecode.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/resourcetable.h"
#include "coregraphics/sampler.h"
#include "coregraphics/shadersemantics.h"
#include "graphics/cameracontext.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "ssaocontext.h"

#include "gpulang/render/system_shaders/hbao_cs.h"
#include "gpulang/render/system_shaders/hbaoblur_cs.h"

#include "frame/default.h"

namespace PostEffects
{
__ImplementPluginContext(PostEffects::SSAOContext);

struct
{
    CoreGraphics::ShaderId hbaoShader, blurShader;
    CoreGraphics::ShaderProgramId xDirectionHBAO, yDirectionHBAO, xDirectionBlur, yDirectionBlur;

    Util::FixedArray<CoreGraphics::ResourceTableId> hbaoTable, blurTableX, blurTableY;
    //CoreGraphics::ResourceTableId hbaoTable, blurTableX, blurTableY;

    // read-write textures
    CoreGraphics::TextureId internalTargets[2];

    struct AOVariables
    {
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

    __bundle.OnWindowResized = SSAOContext::WindowResized;
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);
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
SSAOContext::Setup()
{
    using namespace CoreGraphics;
    CoreGraphics::TextureCreateInfo tinfo;
    tinfo.name = "HBAO-Internal0"_atm;
    tinfo.tag = "system"_atm;
    tinfo.type = Texture2D;
    tinfo.format = PixelFormat::R16G16F;
    tinfo.windowRelative = true;
    tinfo.usage = TextureUsage::ReadWrite;

    ssaoState.internalTargets[0] = CreateTexture(tinfo);
    tinfo.name = "HBAO-Internal1";
    ssaoState.internalTargets[1] = CreateTexture(tinfo);

    // setup shaders
    ssaoState.hbaoShader = ShaderGet("shd:system_shaders/hbao_cs.gplb");
    ssaoState.blurShader = ShaderGet("shd:system_shaders/hbaoblur_cs.gplb");

    ssaoState.xDirectionHBAO = ShaderGetProgram(ssaoState.hbaoShader, ShaderFeatureMask("Alt0"));
    ssaoState.yDirectionHBAO = ShaderGetProgram(ssaoState.hbaoShader, ShaderFeatureMask("Alt1"));
    ssaoState.xDirectionBlur = ShaderGetProgram(ssaoState.blurShader, ShaderFeatureMask("Alt0"));
    ssaoState.yDirectionBlur = ShaderGetProgram(ssaoState.blurShader, ShaderFeatureMask("Alt1"));

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
        ResourceTableSetRWTexture(ssaoState.hbaoTable[i], { ssaoState.internalTargets[0], HbaoCs::HBAO0::BINDING, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetRWTexture(ssaoState.hbaoTable[i], { ssaoState.internalTargets[1], HbaoCs::HBAO1::BINDING, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableCommitChanges(ssaoState.hbaoTable[i]);

        // setup blur table
        ResourceTableSetTexture(ssaoState.blurTableX[i], { ssaoState.internalTargets[1], HbaoblurCs::HBAOX::BINDING, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetRWTexture(ssaoState.blurTableX[i], { ssaoState.internalTargets[0], HbaoblurCs::HBAORG::BINDING, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableCommitChanges(ssaoState.blurTableX[i]);

        ResourceTableSetTexture(ssaoState.blurTableY[i], { ssaoState.internalTargets[0], HbaoblurCs::HBAOY::BINDING, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetRWTexture(ssaoState.blurTableY[i], { FrameScript_default::Texture_SSAOBuffer(), HbaoblurCs::HBAOR::BINDING, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableCommitChanges(ssaoState.blurTableY[i]);
    }


    ssaoState.vars.radius = 12.0f;
    ssaoState.vars.downsample = 1.0f;
    ssaoState.vars.sceneScale = 1.0f;

#define MAX_RADIUS_PIXELS 0.5f
    ssaoState.vars.tanAngleBias = tanf(Math::deg2rad(35.0));
    ssaoState.vars.strength = 1.0f;

    FrameScript_default::Bind_HBAOInternal0(Frame::TextureImport(ssaoState.internalTargets[0]));
    FrameScript_default::Bind_HBAOInternal1(Frame::TextureImport(ssaoState.internalTargets[1]));
    FrameScript_default::RegisterSubgraph_HBAOX_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        uint numGroupsX1 = Math::divandroundup(ssaoState.vars.width, HbaoCs::HBAO_TILE_WIDTH);
        uint numGroupsY2 = ssaoState.vars.height;

        // Compute AO in X
        CoreGraphics::CmdSetShaderProgram(cmdBuf, ssaoState.xDirectionHBAO);
        CoreGraphics::CmdSetResourceTable(cmdBuf, ssaoState.hbaoTable[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
        CoreGraphics::CmdDispatch(cmdBuf, numGroupsX1, numGroupsY2, 1);
    }, nullptr, {
        { FrameScript_default::TextureIndex::ZBuffer, CoreGraphics::PipelineStage::ComputeShaderRead }
        , { FrameScript_default::TextureIndex::HBAOInternal0, CoreGraphics::PipelineStage::ComputeShaderWrite }
    });

    FrameScript_default::RegisterSubgraph_HBAOY_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        uint numGroupsX2 = ssaoState.vars.width;
        uint numGroupsY1 = Math::divandroundup(ssaoState.vars.height, HbaoCs::HBAO_TILE_WIDTH);

        // Compute AO in Y
        CoreGraphics::CmdSetShaderProgram(cmdBuf, ssaoState.yDirectionHBAO);
        CoreGraphics::CmdSetResourceTable(cmdBuf, ssaoState.hbaoTable[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
        CoreGraphics::CmdDispatch(cmdBuf, numGroupsY1, numGroupsX2, 1);
    }, nullptr, {
        { FrameScript_default::TextureIndex::HBAOInternal0, CoreGraphics::PipelineStage::ComputeShaderRead }
        , { FrameScript_default::TextureIndex::HBAOInternal1, CoreGraphics::PipelineStage::ComputeShaderWrite }
    });

    FrameScript_default::RegisterSubgraph_HBAOBlurX_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        uint numGroupsX1 = Math::divandroundup(ssaoState.vars.width, HbaoCs::HBAO_TILE_WIDTH);
        uint numGroupsY2 = ssaoState.vars.height;

        // Compute AO in X
        CoreGraphics::CmdSetShaderProgram(cmdBuf, ssaoState.xDirectionBlur, false);
        CoreGraphics::CmdSetResourceTable(cmdBuf, ssaoState.blurTableX[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
        CoreGraphics::CmdDispatch(cmdBuf, numGroupsX1, numGroupsY2, 1);
    }, nullptr, {
        { FrameScript_default::TextureIndex::HBAOInternal0, CoreGraphics::PipelineStage::ComputeShaderWrite }
        , { FrameScript_default::TextureIndex::HBAOInternal1, CoreGraphics::PipelineStage::ComputeShaderRead }
    });

    FrameScript_default::RegisterSubgraph_HBAOBlurY_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        uint numGroupsX2 = ssaoState.vars.width;
        uint numGroupsY1 = Math::divandroundup(ssaoState.vars.height, HbaoCs::HBAO_TILE_WIDTH);

        // Compute AO in X
        CoreGraphics::CmdSetShaderProgram(cmdBuf, ssaoState.yDirectionBlur, false);
        CoreGraphics::CmdSetResourceTable(cmdBuf, ssaoState.blurTableY[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
        CoreGraphics::CmdDispatch(cmdBuf, numGroupsY1, numGroupsX2, 1);
    }, nullptr, {
        { FrameScript_default::TextureIndex::HBAOInternal0, CoreGraphics::PipelineStage::ComputeShaderRead }
        , { FrameScript_default::TextureIndex::HBAOInternal1, CoreGraphics::PipelineStage::ComputeShaderWrite }
    });
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

    const Math::rectangle<int>& viewport = view->GetViewport();

    ssaoState.vars.width = viewport.width() / ssaoState.vars.downsample;
    ssaoState.vars.height = viewport.height() / ssaoState.vars.downsample;
    ssaoState.vars.maxRadiusPixels = MAX_RADIUS_PIXELS * Math::min(viewport.width(), viewport.height());

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
    ssaoState.vars.focalLength.x = 1.0f / tanf(fov * 0.5f) * (viewport.height() / float(viewport.width()));
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

    HbaoCs::HBAOParams::STRUCT hbaoParams;
    hbaoParams.AOResolution[0] = ssaoState.vars.aoResolution.x;
    hbaoParams.AOResolution[1] = ssaoState.vars.aoResolution.y;
    hbaoParams.InvAOResolution[0] = ssaoState.vars.invAOResolution.x;
    hbaoParams.InvAOResolution[1] = ssaoState.vars.invAOResolution.y;
    hbaoParams.R2 = ssaoState.vars.r2;
    hbaoParams.Strength = ssaoState.vars.strength;
    hbaoParams.TanAngleBias = ssaoState.vars.tanAngleBias;
    hbaoParams.UVToViewA[0] = ssaoState.vars.uvToViewA.x;
    hbaoParams.UVToViewA[1] = ssaoState.vars.uvToViewA.y;
    hbaoParams.UVToViewB[0] = ssaoState.vars.uvToViewB.x;
    hbaoParams.UVToViewB[1] = ssaoState.vars.uvToViewB.y;
    uint64_t hbaoOffset = CoreGraphics::SetConstants(hbaoParams);

    IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

    ResourceTableSetConstantBuffer(ssaoState.hbaoTable[bufferIndex], { CoreGraphics::GetConstantBuffer(bufferIndex), HbaoCs::HBAOParams::BINDING, 0, sizeof(HbaoCs::HBAOParams::STRUCT), hbaoOffset });
    ResourceTableCommitChanges(ssaoState.hbaoTable[bufferIndex]);

    HbaoblurCs::HBAOBlur::STRUCT blurBlock;
    blurBlock.BlurFalloff = ssaoState.vars.blurFalloff;
    blurBlock.BlurDepthThreshold = ssaoState.vars.blurThreshold;
    blurBlock.PowerExponent = 1.5f;
    uint64_t blurOffset = CoreGraphics::SetConstants(blurBlock);

    ResourceTableSetConstantBuffer(ssaoState.blurTableX[bufferIndex], { CoreGraphics::GetConstantBuffer(bufferIndex), HbaoblurCs::HBAOBlur::BINDING, 0, sizeof(HbaoblurCs::HBAOBlur::STRUCT), blurOffset });
    ResourceTableSetConstantBuffer(ssaoState.blurTableY[bufferIndex], { CoreGraphics::GetConstantBuffer(bufferIndex), HbaoblurCs::HBAOBlur::BINDING, 0, sizeof(HbaoblurCs::HBAOBlur::STRUCT), blurOffset });
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
    TextureWindowResized(ssaoState.internalTargets[0]);
    TextureWindowResized(ssaoState.internalTargets[1]);

    FrameScript_default::Bind_HBAOInternal0(Frame::TextureImport(ssaoState.internalTargets[0]));
    FrameScript_default::Bind_HBAOInternal1(Frame::TextureImport(ssaoState.internalTargets[1]));

    IndexT i;
    for (i = 0; i < ssaoState.hbaoTable.Size(); i++)
    {
        // setup hbao table
        ResourceTableSetRWTexture(ssaoState.hbaoTable[i], { ssaoState.internalTargets[0], HbaoCs::HBAO0::BINDING, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetRWTexture(ssaoState.hbaoTable[i], { ssaoState.internalTargets[1], HbaoCs::HBAO1::BINDING, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableCommitChanges(ssaoState.hbaoTable[i]);

        // setup blur table
        ResourceTableSetTexture(ssaoState.blurTableX[i], { ssaoState.internalTargets[1], HbaoblurCs::HBAOX::BINDING, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetRWTexture(ssaoState.blurTableX[i], { ssaoState.internalTargets[0], HbaoblurCs::HBAORG::BINDING, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableCommitChanges(ssaoState.blurTableX[i]);

        ResourceTableSetTexture(ssaoState.blurTableY[i], { ssaoState.internalTargets[0], HbaoblurCs::HBAOY::BINDING, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetRWTexture(ssaoState.blurTableY[i], { FrameScript_default::Texture_SSAOBuffer(), HbaoblurCs::HBAOR::BINDING, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableCommitChanges(ssaoState.blurTableY[i]);
    }
}

} // namespace PostEffects
