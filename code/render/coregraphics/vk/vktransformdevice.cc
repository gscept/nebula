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
	
	ShaderId shader = ShaderGet("shd:shared.fxb"_atm);
	this->viewTable = ShaderCreateResourceTable(shader, NEBULA_FRAME_GROUP);
	this->viewConstants = ShaderCreateConstantBuffer(shader, "FrameBlock");
	IndexT viewConstantsSlot = ShaderGetResourceSlot(shader, "FrameBlock");
	ResourceTableSetConstantBuffer(this->viewTable, { this->viewConstants, viewConstantsSlot, 0, false, false, -1, 0});
	ResourceTableCommitChanges(this->viewTable);
	this->tableLayout = ShaderGetResourcePipeline(shader);

	// setup camera block, update once per frame - no need to sync
	this->viewVar = ShaderGetConstantBinding(shader, NEBULA_SEMANTIC_VIEW);
	this->invViewVar = ShaderGetConstantBinding(shader, NEBULA_SEMANTIC_INVVIEW);
	this->viewProjVar = ShaderGetConstantBinding(shader, NEBULA_SEMANTIC_VIEWPROJECTION);
	this->invViewProjVar = ShaderGetConstantBinding(shader, NEBULA_SEMANTIC_INVVIEWPROJECTION);
	this->projVar = ShaderGetConstantBinding(shader, NEBULA_SEMANTIC_PROJECTION);
	this->invProjVar = ShaderGetConstantBinding(shader, NEBULA_SEMANTIC_INVPROJECTION);
	this->eyePosVar = ShaderGetConstantBinding(shader, NEBULA_SEMANTIC_EYEPOS);
	this->focalLengthVar = ShaderGetConstantBinding(shader, NEBULA_SEMANTIC_FOCALLENGTH);
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

	ConstantBufferUpdate(this->viewConstants, this->GetViewProjTransform(), this->viewProjVar);
	ConstantBufferUpdate(this->viewConstants, this->GetInvViewTransform(), this->invViewProjVar);
	ConstantBufferUpdate(this->viewConstants, this->GetViewTransform(), this->viewVar);
	ConstantBufferUpdate(this->viewConstants, this->GetInvViewTransform(), this->invViewVar);
	ConstantBufferUpdate(this->viewConstants, this->GetProjTransform(), this->projVar);
	ConstantBufferUpdate(this->viewConstants, this->GetInvProjTransform(), this->invProjVar);
	ConstantBufferUpdate(this->viewConstants, this->GetInvViewTransform().getrow3(), this->eyePosVar);
	ConstantBufferUpdate(this->viewConstants, float4(this->GetFocalLength().x(), this->GetFocalLength().y(), 0, 0), this->focalLengthVar);
	ConstantBufferUpdate(this->viewConstants, float4((float)FrameSync::FrameSyncTimer::Instance()->GetTime(), Math::n_rand(0, 1), (float)FrameSync::FrameSyncTimer::Instance()->GetFrameTime(), 0), this->timeAndRandomVar);
}

} // namespace Vulkan