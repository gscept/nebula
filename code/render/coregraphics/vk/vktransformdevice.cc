//------------------------------------------------------------------------------
// vktransformdevice.cc
// (C) 2016 Individual contributors, see AUTHORS file
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
	
	this->sharedShader = ShaderCreateSharedState("shd:shared.fxb"_atm, { NEBULAT_FRAME_GROUP });

	// setup camera block, update once per frame - no need to sync
	this->viewVar = ShaderStateGetConstant(this->sharedShader, NEBULA3_SEMANTIC_VIEW);
	this->invViewVar = ShaderStateGetConstant(this->sharedShader, NEBULA3_SEMANTIC_INVVIEW);
	this->viewProjVar = ShaderStateGetConstant(this->sharedShader, NEBULA3_SEMANTIC_VIEWPROJECTION);
	this->invViewProjVar = ShaderStateGetConstant(this->sharedShader, NEBULA3_SEMANTIC_INVVIEWPROJECTION);
	this->projVar = ShaderStateGetConstant(this->sharedShader, NEBULA3_SEMANTIC_PROJECTION);
	this->invProjVar = ShaderStateGetConstant(this->sharedShader, NEBULA3_SEMANTIC_INVPROJECTION);
	this->eyePosVar = ShaderStateGetConstant(this->sharedShader, NEBULA3_SEMANTIC_EYEPOS);
	this->focalLengthVar = ShaderStateGetConstant(this->sharedShader, NEBULA3_SEMANTIC_FOCALLENGTH);
	this->timeAndRandomVar = ShaderStateGetConstant(this->sharedShader, NEBULA3_SEMANTIC_TIMEANDRANDOM);

	return TransformDeviceBase::Open();
}

//------------------------------------------------------------------------------
/**
*/
void
VkTransformDevice::Close()
{
	ShaderDestroyState(this->sharedShader);
	this->sharedShader = ShaderStateId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
void
VkTransformDevice::ApplyViewSettings()
{
	TransformDeviceBase::ApplyViewSettings();

	ShaderConstantSet(this->viewProjVar, this->sharedShader, this->GetViewProjTransform());
	ShaderConstantSet(this->invViewProjVar, this->sharedShader, this->GetInvViewTransform());
	ShaderConstantSet(this->viewVar, this->sharedShader, this->GetViewTransform());
	ShaderConstantSet(this->invViewVar, this->sharedShader, this->GetInvViewTransform());
	ShaderConstantSet(this->projVar, this->sharedShader, this->GetProjTransform());
	ShaderConstantSet(this->invProjVar, this->sharedShader, this->GetInvProjTransform());
	ShaderConstantSet(this->eyePosVar, this->sharedShader, this->GetInvViewTransform().getrow3());
	ShaderConstantSet(this->focalLengthVar, this->sharedShader, float4(this->GetFocalLength().x(), this->GetFocalLength().y(), 0, 0));
	ShaderConstantSet(this->timeAndRandomVar, this->sharedShader, float4((float)FrameSync::FrameSyncTimer::Instance()->GetTime(), Math::n_rand(0, 1), 0, 0));
	ShaderStateCommit(this->sharedShader);
}

} // namespace Vulkan