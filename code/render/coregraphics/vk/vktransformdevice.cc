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
	DestroyResourceTable(this->viewTable);
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

	// allocate memory from global buffer
	uint offset = CoreGraphics::AllocateGraphicsConstantBufferMemory(MainThreadConstantBuffer, sizeof(Shared::FrameBlock));

	// update block structure
	alignas(16) Shared::FrameBlock block;
	this->GetViewTransform().store(block.View);
	this->GetViewProjTransform().store(block.ViewProjection);
	this->GetProjTransform().store(block.Projection);
	this->GetInvViewTransform().store(block.InvView);
	this->GetInvProjTransform().store(block.InvProjection);
	this->GetInvViewTransform().store(block.InvViewProjection);
	this->GetInvViewTransform().getrow3().store(block.EyePos);
	float4(this->GetFocalLength().x(), this->GetFocalLength().y(), this->GetNearFarPlane().x(), this->GetNearFarPlane().y()).store(block.FocalLengthNearFar);
	float4((float)FrameSync::FrameSyncTimer::Instance()->GetTime(), Math::n_rand(0, 1), (float)FrameSync::FrameSyncTimer::Instance()->GetFrameTime(), 0).store(block.TimeAndRandom);

	// update actual constant buffer
	ConstantBufferUpdate(this->viewConstants, block, offset);
	frameOffset = offset;

	// update resource table
	IndexT bufferedFrameIndex = GetBufferedFrameIndex();
	ResourceTableSetConstantBuffer(this->viewTables[bufferedFrameIndex], { this->viewConstants, this->viewConstantsSlot, 0, false, false, sizeof(Shared::FrameBlock), (SizeT)offset });
	ResourceTableCommitChanges(this->viewTables[bufferedFrameIndex]);
}

} // namespace Vulkan