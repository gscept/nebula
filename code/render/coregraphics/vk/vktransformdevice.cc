//------------------------------------------------------------------------------
// vktransformdevice.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vktransformdevice.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/shader.h"
#include "coregraphics/config.h"
#include "coregraphics/shadersemantics.h"
#include "framesync/framesynctimer.h"

#include "shared.h"

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
	}

	this->viewConstants = CoreGraphics::GetGraphicsConstantBuffer(MainThreadConstantBuffer);
	this->viewConstantsSlot = ShaderGetResourceSlot(shader, "FrameBlock");
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
	DestroyConstantBuffer(this->viewConstants);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkTransformDevice::SetProjTransform(const Math::matrix44& m)
{
	Math::matrix44 conv = m;
	conv.setrow1(float4::multiply(conv.getrow1(), float4(-1)));
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
	Math::matrix44::storeu(this->GetViewTransform(), block.View);
	Math::matrix44::storeu(this->GetViewProjTransform(), block.ViewProjection);
	Math::matrix44::storeu(this->GetProjTransform(), block.Projection);
	Math::matrix44::storeu(this->GetInvViewTransform(), block.InvView);
	Math::matrix44::storeu(this->GetInvProjTransform(), block.InvProjection);
	Math::matrix44::storeu(Math::matrix44::inverse(this->GetViewProjTransform()), block.InvViewProjection);
	Math::float4::storeu(this->GetInvViewTransform().getrow3(), block.EyePos);
	Math::float4::storeu(float4(this->GetFocalLength().x(), this->GetFocalLength().y(), this->GetNearFarPlane().x(), this->GetNearFarPlane().y()), block.FocalLengthNearFar);
	Math::float4::storeu(float4((float)FrameSync::FrameSyncTimer::Instance()->GetTime(), Math::n_rand(0, 1), (float)FrameSync::FrameSyncTimer::Instance()->GetFrameTime(), 0), block.TimeAndRandom);
	uint offset = CoreGraphics::SetGraphicsConstants(MainThreadConstantBuffer, block);

	// update actual constant buffer
	frameOffset = offset;

	// update resource table
	IndexT bufferedFrameIndex = GetBufferedFrameIndex();
	ResourceTableSetConstantBuffer(this->viewTables[bufferedFrameIndex], { this->viewConstants, this->viewConstantsSlot, 0, false, false, sizeof(Shared::FrameBlock), (SizeT)offset });
	ResourceTableCommitChanges(this->viewTables[bufferedFrameIndex]);
}

} // namespace Vulkan