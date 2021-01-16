//------------------------------------------------------------------------------
// vktransformdevice.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vktransformdevice.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/buffer.h"
#include "coregraphics/shader.h"
#include "coregraphics/config.h"
#include "coregraphics/shadersemantics.h"
#include "framesync/framesynctimer.h"


using namespace Util;
using namespace CoreGraphics;
using namespace Math;
namespace Vulkan
{

__ImplementClass(Vulkan::VkTransformDevice, 'VKTD', Base::TransformDeviceBase);
__ImplementSingleton(Vulkan::VkTransformDevice);
//------------------------------------------------------------------------------
/**
*/
VkTransformDevice::VkTransformDevice()
    : viewVar(InvalidIndex)
    , invViewVar(InvalidIndex)
    , viewProjVar(InvalidIndex)
    , invViewProjVar(InvalidIndex)
    , projVar(InvalidIndex)
    , invProjVar(InvalidIndex)
    , eyePosVar(InvalidIndex)
    , focalLengthNearFarVar(InvalidIndex)
    , viewMatricesVar(InvalidIndex)
    , timeAndRandomVar(InvalidIndex)
    , nearFarPlaneVar(InvalidIndex)
    , shadowCameraBlockVar(InvalidIndex)
    , viewConstants(CoreGraphics::BufferId::Invalid())
    , viewConstantsSlot(InvalidIndex)
    , shadowConstantsSlot(InvalidIndex)
    , tableLayout(CoreGraphics::ResourcePipelineId::Invalid())
{
    __ConstructSingleton
}

//------------------------------------------------------------------------------
/**
*/
VkTransformDevice::~VkTransformDevice()
{
    __DestructSingleton
}

//------------------------------------------------------------------------------
/**
*/
bool
VkTransformDevice::Open()
{
    ShaderServer* shdServer = ShaderServer::Instance();

    // setup tables
    ShaderId shader = ShaderGet("shd:shared.fxb"_atm);
    this->viewTables.Resize(CoreGraphics::GetNumBufferedFrames());
    IndexT i;
    for (i = 0; i < this->viewTables.Size(); i++)
    {
        this->viewTables[i] = ShaderCreateResourceTable(shader, NEBULA_FRAME_GROUP);
		CoreGraphics::ObjectSetName(this->viewTables[i], "Main Frame Group Descriptor");
    }

    this->viewConstants = CoreGraphics::GetGraphicsConstantBuffer(MainThreadConstantBuffer);
    this->viewConstantsSlot = ShaderGetResourceSlot(shader, "FrameBlock");
    this->shadowConstantsSlot = ShaderGetResourceSlot(shader, "ShadowMatrixBlock");
    this->tableLayout = ShaderGetResourcePipeline(shader);

    // setup camera block, update once per frame - no need to sync
    this->viewVar = ShaderGetConstantBinding(shader, NEBULA_SEMANTIC_VIEW);
    this->invViewVar = ShaderGetConstantBinding(shader, NEBULA_SEMANTIC_INVVIEW);
    this->viewProjVar = ShaderGetConstantBinding(shader, NEBULA_SEMANTIC_VIEWPROJECTION);
    this->invViewProjVar = ShaderGetConstantBinding(shader, NEBULA_SEMANTIC_INVVIEWPROJECTION);
    this->projVar = ShaderGetConstantBinding(shader, NEBULA_SEMANTIC_PROJECTION);
    this->invProjVar = ShaderGetConstantBinding(shader, NEBULA_SEMANTIC_INVPROJECTION);
    this->eyePosVar = ShaderGetConstantBinding(shader, NEBULA_SEMANTIC_EYEPOS);
    this->focalLengthNearFarVar = ShaderGetConstantBinding(shader, NEBULA_SEMANTIC_FOCALLENGTHNEARFAR);
    this->timeAndRandomVar = ShaderGetConstantBinding(shader, NEBULA_SEMANTIC_TIMEANDRANDOM);

    return TransformDeviceBase::Open();
}

//------------------------------------------------------------------------------
/**
*/
void
VkTransformDevice::Close()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void 
VkTransformDevice::SetProjTransform(const Math::mat4& m)
{
    Math::mat4 conv = m;
    conv.row1 = -conv.row1;
    TransformDeviceBase::SetProjTransform(conv);
}

//------------------------------------------------------------------------------
/**
*/
void
VkTransformDevice::ApplyViewSettings()
{
    TransformDeviceBase::ApplyViewSettings();

    // update block structure
    alignas(16) Shared::FrameBlock block;
    this->GetViewTransform().store(block.View);
    this->GetViewProjTransform().store(block.ViewProjection);
    this->GetProjTransform().store(block.Projection);
    this->GetInvViewTransform().store(block.InvView);
    this->GetInvProjTransform().store(block.InvProjection);
    Math::inverse(this->GetViewProjTransform()).store(block.InvViewProjection);
    this->GetInvViewTransform().position.store(block.EyePos);
    vec4(this->GetFocalLength().x, this->GetFocalLength().y, this->GetNearFarPlane().x, this->GetNearFarPlane().y).store(block.FocalLengthNearFar);
    vec4((float)FrameSync::FrameSyncTimer::Instance()->GetTime(), Math::rand(0, 1), (float)FrameSync::FrameSyncTimer::Instance()->GetFrameTime(), 0).store(block.TimeAndRandom);
    uint offset = CoreGraphics::SetGraphicsConstants(MainThreadConstantBuffer, block);

    // update resource table
    IndexT bufferedFrameIndex = GetBufferedFrameIndex();
    ResourceTableSetConstantBuffer(this->viewTables[bufferedFrameIndex], { this->viewConstants, this->viewConstantsSlot, 0, false, false, sizeof(Shared::FrameBlock), (SizeT)offset });
    ResourceTableCommitChanges(this->viewTables[bufferedFrameIndex]);

	CoreGraphics::SetFrameResourceTable(this->viewTables[bufferedFrameIndex]);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkTransformDevice::ApplyShadowSettings(const Shared::ShadowMatrixBlock& block)
{
    uint offset = CoreGraphics::SetGraphicsConstants(MainThreadConstantBuffer, block);

    IndexT bufferedFrameIndex = GetBufferedFrameIndex();
    ResourceTableSetConstantBuffer(this->viewTables[bufferedFrameIndex], { this->viewConstants, this->shadowConstantsSlot, 0, false, false, sizeof(Shared::ShadowMatrixBlock), (SizeT)offset });
    ResourceTableCommitChanges(this->viewTables[bufferedFrameIndex]);

	CoreGraphics::SetFrameResourceTable(this->viewTables[bufferedFrameIndex]);
}

} // namespace Vulkan
