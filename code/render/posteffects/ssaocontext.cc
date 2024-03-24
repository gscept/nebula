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

#include "system_shaders/hbao_cs.h"
#include "system_shaders/hbaoblur_cs.h"

namespace PostEffects
{
__ImplementPluginContext(PostEffects::SSAOContext);

struct
{
    CoreGraphics::ShaderId hbaoShader, blurShader;
    CoreGraphics::ShaderProgramId xDirectionHBAO, yDirectionHBAO, xDirectionBlur, yDirectionBlur;

    Util::FixedArray<CoreGraphics::ResourceTableId> hbaoTable, blurTableX, blurTableY;
    //CoreGraphics::ResourceTableId hbaoTable, blurTableX, blurTableY;

    IndexT uvToViewAVar, uvToViewBVar, r2Var,
        aoResolutionVar, invAOResolutionVar, strengthVar, tanAngleBiasVar,
        powerExponentVar, blurFalloff, blurDepthThreshold;

    // read-write textures
    CoreGraphics::TextureId internalTargets[2];
    CoreGraphics::TextureId ssaoOutput;
    CoreGraphics::TextureId zBuffer;

    Memory::ArenaAllocator<sizeof(Frame::FrameCode) * 4> frameOpAllocator;

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

    // setup shaders
    ssaoState.hbaoShader = ShaderGet("shd:system_shaders/hbao_cs.fxb");
    ssaoState.blurShader = ShaderGet("shd:system_shaders/hbaoblur_cs.fxb");

    ssaoState.xDirectionHBAO = ShaderGetProgram(ssaoState.hbaoShader, ShaderFeatureMask("Alt0"));
    ssaoState.yDirectionHBAO = ShaderGetProgram(ssaoState.hbaoShader, ShaderFeatureMask("Alt1"));
    ssaoState.xDirectionBlur = ShaderGetProgram(ssaoState.blurShader, ShaderFeatureMask("Alt0"));
    ssaoState.yDirectionBlur = ShaderGetProgram(ssaoState.blurShader, ShaderFeatureMask("Alt1"));

    ssaoState.ssaoOutput = script->GetTexture("SSAOBuffer");
    ssaoState.zBuffer = script->GetTexture("ZBuffer");
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
        ResourceTableSetRWTexture(ssaoState.hbaoTable[i], { ssaoState.internalTargets[0], HbaoCs::Table_Batch::HBAO0_SLOT, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetRWTexture(ssaoState.hbaoTable[i], { ssaoState.internalTargets[1], HbaoCs::Table_Batch::HBAO1_SLOT, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableCommitChanges(ssaoState.hbaoTable[i]);

        // setup blur table
        ResourceTableSetTexture(ssaoState.blurTableX[i], { ssaoState.internalTargets[1], HbaoblurCs::Table_Batch::HBAOX_SLOT, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetRWTexture(ssaoState.blurTableX[i], { ssaoState.internalTargets[0], HbaoblurCs::Table_Batch::HBAORG_SLOT, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableCommitChanges(ssaoState.blurTableX[i]);

        ResourceTableSetTexture(ssaoState.blurTableY[i], { ssaoState.internalTargets[0], HbaoblurCs::Table_Batch::HBAOY_SLOT, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetRWTexture(ssaoState.blurTableY[i], { ssaoState.ssaoOutput, HbaoblurCs::Table_Batch::HBAOR_SLOT, 0, CoreGraphics::InvalidSamplerId });
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

    // Construct subgraph
    auto aoX = ssaoState.frameOpAllocator.Alloc<Frame::FrameCode>();
    aoX->SetName("HBAO X");
    aoX->domain = CoreGraphics::BarrierDomain::Global;
    aoX->textureDeps.Add(ssaoState.zBuffer,
                         {
                            "ZBuffer"
                            , CoreGraphics::PipelineStage::ComputeShaderRead
                            , CoreGraphics::TextureSubresourceInfo::DepthStencil(ssaoState.zBuffer)
                         });
    aoX->textureDeps.Add(ssaoState.internalTargets[0],
                         {
                            "SSAOBuffer0"
                            , CoreGraphics::PipelineStage::ComputeShaderWrite
                            , CoreGraphics::TextureSubresourceInfo::Color(ssaoState.internalTargets[0])
                         });
    aoX->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        uint numGroupsX1 = Math::divandroundup(ssaoState.vars.width, HbaoCs::HBAOTileWidth);
        uint numGroupsY2 = ssaoState.vars.height;

        // Compute AO in X
        CoreGraphics::CmdSetShaderProgram(cmdBuf, ssaoState.xDirectionHBAO);
        CoreGraphics::CmdSetResourceTable(cmdBuf, ssaoState.hbaoTable[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
        CoreGraphics::CmdDispatch(cmdBuf, numGroupsX1, numGroupsY2, 1);
    };

    auto aoY = ssaoState.frameOpAllocator.Alloc<Frame::FrameCode>();
    aoY->SetName("HBAO Y");
    aoY->domain = CoreGraphics::BarrierDomain::Global;
    aoY->textureDeps.Add(ssaoState.internalTargets[0],
                         {
                            "SSAOBuffer0"
                            , CoreGraphics::PipelineStage::ComputeShaderRead
                            , CoreGraphics::TextureSubresourceInfo::Color(ssaoState.internalTargets[0])
                         });
    aoY->textureDeps.Add(ssaoState.internalTargets[1],
                         {
                            "SSAOBuffer1"
                            , CoreGraphics::PipelineStage::ComputeShaderWrite
                            , CoreGraphics::TextureSubresourceInfo::Color(ssaoState.internalTargets[1])
                         });
    aoY->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        uint numGroupsX2 = ssaoState.vars.width;
        uint numGroupsY1 = Math::divandroundup(ssaoState.vars.height, HbaoCs::HBAOTileWidth);

        // Compute AO in X
        CoreGraphics::CmdSetShaderProgram(cmdBuf, ssaoState.xDirectionHBAO);
        CoreGraphics::CmdSetResourceTable(cmdBuf, ssaoState.hbaoTable[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
        CoreGraphics::CmdDispatch(cmdBuf, numGroupsY1, numGroupsX2, 1);
    };

    auto blurX = ssaoState.frameOpAllocator.Alloc<Frame::FrameCode>();
    blurX->SetName("HBAO Blur X");
    blurX->domain = CoreGraphics::BarrierDomain::Global;
    blurX->textureDeps.Add(ssaoState.internalTargets[1],
                         {
                            "SSAOBuffer1"
                            , CoreGraphics::PipelineStage::ComputeShaderRead
                            , CoreGraphics::TextureSubresourceInfo::Color(ssaoState.internalTargets[1])
                         });
    blurX->textureDeps.Add(ssaoState.internalTargets[0],
                         {
                            "SSAOBuffer0"
                            , CoreGraphics::PipelineStage::ComputeShaderWrite
                            , CoreGraphics::TextureSubresourceInfo::Color(ssaoState.internalTargets[0])
                         });
    blurX->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        uint numGroupsX1 = Math::divandroundup(ssaoState.vars.width, HbaoCs::HBAOTileWidth);
        uint numGroupsY2 = ssaoState.vars.height;

        // Compute AO in X
        CoreGraphics::CmdSetShaderProgram(cmdBuf, ssaoState.xDirectionBlur, false);
        CoreGraphics::CmdSetResourceTable(cmdBuf, ssaoState.blurTableX[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
        CoreGraphics::CmdDispatch(cmdBuf, numGroupsX1, numGroupsY2, 1);
    };

    auto blurY = ssaoState.frameOpAllocator.Alloc<Frame::FrameCode>();
    blurY->SetName("HBAO Blur X");
    blurY->domain = CoreGraphics::BarrierDomain::Global;
    blurY->textureDeps.Add(ssaoState.internalTargets[0],
                         {
                            "SSAOBuffer1"
                            , CoreGraphics::PipelineStage::ComputeShaderRead
                            , CoreGraphics::TextureSubresourceInfo::Color(ssaoState.internalTargets[0])
                         });
    blurY->textureDeps.Add(ssaoState.ssaoOutput,
                         {
                            "SSAOOutput"
                            , CoreGraphics::PipelineStage::ComputeShaderWrite
                            , CoreGraphics::TextureSubresourceInfo::Color(ssaoState.internalTargets[1])
                         });
    blurY->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        uint numGroupsX2 = ssaoState.vars.width;
        uint numGroupsY1 = Math::divandroundup(ssaoState.vars.height, HbaoCs::HBAOTileWidth);

        // Compute AO in X
        CoreGraphics::CmdSetShaderProgram(cmdBuf, ssaoState.yDirectionBlur, false);
        CoreGraphics::CmdSetResourceTable(cmdBuf, ssaoState.blurTableY[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
        CoreGraphics::CmdDispatch(cmdBuf, numGroupsY1, numGroupsX2, 1);
    };

    Frame::AddSubgraph("HBAO", { aoX, aoY, blurX, blurY });
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
    uint hbaoOffset = CoreGraphics::SetConstants(hbaoBlock);

    IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

    ResourceTableSetConstantBuffer(ssaoState.hbaoTable[bufferIndex], { CoreGraphics::GetConstantBuffer(bufferIndex), HbaoCs::Table_Batch::HBAOBlock_SLOT, 0, sizeof(HbaoCs::HBAOBlock), (SizeT)hbaoOffset });
    ResourceTableCommitChanges(ssaoState.hbaoTable[bufferIndex]);

    HbaoblurCs::HBAOBlur blurBlock;
    blurBlock.BlurFalloff = ssaoState.vars.blurFalloff;
    blurBlock.BlurDepthThreshold = ssaoState.vars.blurThreshold;
    blurBlock.PowerExponent = 1.5f;
    uint blurOffset = CoreGraphics::SetConstants(blurBlock);

    ResourceTableSetConstantBuffer(ssaoState.blurTableX[bufferIndex], { CoreGraphics::GetConstantBuffer(bufferIndex), HbaoblurCs::Table_Batch::HBAOBlur_SLOT, 0, sizeof(HbaoblurCs::HBAOBlur), (SizeT)blurOffset });
    ResourceTableSetConstantBuffer(ssaoState.blurTableY[bufferIndex], { CoreGraphics::GetConstantBuffer(bufferIndex), HbaoblurCs::Table_Batch::HBAOBlur_SLOT, 0, sizeof(HbaoblurCs::HBAOBlur), (SizeT)blurOffset });
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

    IndexT i;
    for (i = 0; i < ssaoState.hbaoTable.Size(); i++)
    {
        // setup hbao table
        ResourceTableSetRWTexture(ssaoState.hbaoTable[i], { ssaoState.internalTargets[0], HbaoCs::Table_Batch::HBAO0_SLOT, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetRWTexture(ssaoState.hbaoTable[i], { ssaoState.internalTargets[1], HbaoCs::Table_Batch::HBAO1_SLOT, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableCommitChanges(ssaoState.hbaoTable[i]);

        // setup blur table
        ResourceTableSetTexture(ssaoState.blurTableX[i], { ssaoState.internalTargets[1], HbaoblurCs::Table_Batch::HBAOX_SLOT, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetRWTexture(ssaoState.blurTableX[i], { ssaoState.internalTargets[0], HbaoblurCs::Table_Batch::HBAORG_SLOT, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableCommitChanges(ssaoState.blurTableX[i]);

        ResourceTableSetTexture(ssaoState.blurTableY[i], { ssaoState.internalTargets[0], HbaoblurCs::Table_Batch::HBAOY_SLOT, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetRWTexture(ssaoState.blurTableY[i], { ssaoState.ssaoOutput, HbaoblurCs::Table_Batch::HBAOR_SLOT, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableCommitChanges(ssaoState.blurTableY[i]);
    }

    TextureDimensions dims = TextureGetDimensions(ssaoState.ssaoOutput);
    ssaoState.vars.fullWidth = (float)dims.width;
    ssaoState.vars.fullHeight = (float)dims.height;
    ssaoState.vars.radius = 12.0f;
    ssaoState.vars.downsample = 1.0f;
    ssaoState.vars.sceneScale = 1.0f;

    ssaoState.vars.maxRadiusPixels = MAX_RADIUS_PIXELS * Math::min(ssaoState.vars.fullWidth, ssaoState.vars.fullHeight);
}

} // namespace PostEffects
