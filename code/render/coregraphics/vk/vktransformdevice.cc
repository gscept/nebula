//------------------------------------------------------------------------------
// vktransformdevice.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vktransformdevice.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/shaderstate.h"
#include "coregraphics/shadervariable.h"
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
	this->sharedShader = shdServer->CreateSharedShaderState("shd:shared", { NEBULAT_FRAME_GROUP });

	// setup camera block, update once per frame - no need to sync
	this->viewVar = this->sharedShader->GetVariableByName(NEBULA3_SEMANTIC_VIEW);
	this->invViewVar = this->sharedShader->GetVariableByName(NEBULA3_SEMANTIC_INVVIEW);
	this->viewProjVar = this->sharedShader->GetVariableByName(NEBULA3_SEMANTIC_VIEWPROJECTION);
	this->invViewProjVar = this->sharedShader->GetVariableByName(NEBULA3_SEMANTIC_INVVIEWPROJECTION);
	this->projVar = this->sharedShader->GetVariableByName(NEBULA3_SEMANTIC_PROJECTION);
	this->invProjVar = this->sharedShader->GetVariableByName(NEBULA3_SEMANTIC_INVPROJECTION);
	this->eyePosVar = this->sharedShader->GetVariableByName(NEBULA3_SEMANTIC_EYEPOS);
	this->focalLengthVar = this->sharedShader->GetVariableByName(NEBULA3_SEMANTIC_FOCALLENGTH);
	this->timeAndRandomVar = this->sharedShader->GetVariableByName(NEBULA3_SEMANTIC_TIMEANDRANDOM);

	return TransformDeviceBase::Open();
}

//------------------------------------------------------------------------------
/**
*/
void
VkTransformDevice::Close()
{
	this->sharedShader->Discard();
	this->sharedShader = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
VkTransformDevice::ApplyViewSettings()
{
	TransformDeviceBase::ApplyViewSettings();

	// update per frame view stuff
	this->viewProjVar->SetMatrix(this->GetViewProjTransform());
	this->invViewProjVar->SetMatrix(this->GetInvViewTransform());
	this->viewVar->SetMatrix(this->GetViewTransform());
	this->invViewVar->SetMatrix(this->GetInvViewTransform());
	this->projVar->SetMatrix(this->GetProjTransform());
	this->invProjVar->SetMatrix(this->GetInvProjTransform());
	this->eyePosVar->SetFloat4(this->GetInvViewTransform().getrow3());
	this->focalLengthVar->SetFloat4(float4(this->GetFocalLength().x(), this->GetFocalLength().y(), 0, 0));

	// set time and random, this isn't really related to the transform device, but the variable is in the per-frame block
	this->timeAndRandomVar->SetFloat4(Math::float4(
		(float)FrameSync::FrameSyncTimer::Instance()->GetTime(),
		Math::n_rand(0, 1),
		0, 0));
}

//------------------------------------------------------------------------------
/**
*/
void
VkTransformDevice::ApplyModelTransforms(const Ptr<CoreGraphics::ShaderState>& shdInst)
{
	if (shdInst->HasVariableByName(NEBULA3_SEMANTIC_MODEL))
	{
		shdInst->GetVariableByName(NEBULA3_SEMANTIC_MODEL)->SetMatrix(this->GetModelTransform());
	}
	if (shdInst->HasVariableByName(NEBULA3_SEMANTIC_INVMODEL))
	{
		shdInst->GetVariableByName(NEBULA3_SEMANTIC_INVMODEL)->SetMatrix(this->GetInvModelTransform());
	}
}

} // namespace Vulkan