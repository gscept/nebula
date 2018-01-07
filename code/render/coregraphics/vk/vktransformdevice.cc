//------------------------------------------------------------------------------
// vktransformdevice.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vktransformdevice.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/shader.h"
#include "coregraphics/config.h"
#include "coregraphics/shadersemantics.h"
#include "framesync/framesynctimer.h"
#include "coregraphics/renderdevice.h"


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
	
	this->sharedShader = ShaderCreateSharedState("shd:shared"_atm, { NEBULAT_FRAME_GROUP });

	// setup camera block, update once per frame - no need to sync
	this->viewVar = ShaderStateGetVariable(this->sharedShader, NEBULA3_SEMANTIC_VIEW);
	this->invViewVar = ShaderStateGetVariable(this->sharedShader, NEBULA3_SEMANTIC_INVVIEW);
	this->viewProjVar = ShaderStateGetVariable(this->sharedShader, NEBULA3_SEMANTIC_VIEWPROJECTION);
	this->invViewProjVar = ShaderStateGetVariable(this->sharedShader, NEBULA3_SEMANTIC_INVVIEWPROJECTION);
	this->projVar = ShaderStateGetVariable(this->sharedShader, NEBULA3_SEMANTIC_PROJECTION);
	this->invProjVar = ShaderStateGetVariable(this->sharedShader, NEBULA3_SEMANTIC_INVPROJECTION);
	this->eyePosVar = ShaderStateGetVariable(this->sharedShader, NEBULA3_SEMANTIC_EYEPOS);
	this->focalLengthVar = ShaderStateGetVariable(this->sharedShader, NEBULA3_SEMANTIC_FOCALLENGTH);
	this->timeAndRandomVar = ShaderStateGetVariable(this->sharedShader, NEBULA3_SEMANTIC_TIMEANDRANDOM);

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

	ShaderVariableSet(this->viewProjVar, this->sharedShader, this->GetViewProjTransform());
	ShaderVariableSet(this->invViewProjVar, this->sharedShader, this->GetInvViewTransform());
	ShaderVariableSet(this->viewVar, this->sharedShader, this->GetViewTransform());
	ShaderVariableSet(this->invViewVar, this->sharedShader, this->GetInvViewTransform());
	ShaderVariableSet(this->projVar, this->sharedShader, this->GetProjTransform());
	ShaderVariableSet(this->invProjVar, this->sharedShader, this->GetInvProjTransform());
	ShaderVariableSet(this->eyePosVar, this->sharedShader, this->GetInvViewTransform().getrow3());
	ShaderVariableSet(this->focalLengthVar, this->sharedShader, float4(this->GetFocalLength().x(), this->GetFocalLength().y(), 0, 0));
	ShaderVariableSet(this->timeAndRandomVar, this->sharedShader, float4((float)FrameSync::FrameSyncTimer::Instance()->GetTime(), Math::n_rand(0, 1), 0, 0));
}

} // namespace Vulkan