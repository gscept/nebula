//------------------------------------------------------------------------------
//  ogl4transformdevice.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/ogl4/ogl4transformdevice.h"
#include "coregraphics/displaydevice.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/shadersemantics.h"
#include "coregraphics/ogl4/ogl4renderdevice.h"
#include "framesync/framesynctimer.h"
#include "lighting/shadowserver.h"
#include "lighting/lightserver.h"
#include "ogl4uniformbuffer.h"
#include "coregraphics/constantbuffer.h"

namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4TransformDevice, 'D1TD', Base::TransformDeviceBase);
__ImplementSingleton(OpenGL4::OGL4TransformDevice);

using namespace Util;
using namespace CoreGraphics;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
OGL4TransformDevice::OGL4TransformDevice()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
OGL4TransformDevice::~OGL4TransformDevice()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
OGL4TransformDevice::Open()
{
    ShaderServer* shdServer = ShaderServer::Instance();
    this->sharedShader = shdServer->GetSharedShader();

    // setup camera block, update once per frame - no need to sync
    this->cameraBuffer = ConstantBuffer::Create();
	this->cameraBuffer->SetupFromBlockInShader(this->sharedShader, "CameraBlock");
    this->viewVar = this->cameraBuffer->GetVariableByName(NEBULA_SEMANTIC_VIEW);
    this->invViewVar = this->cameraBuffer->GetVariableByName(NEBULA_SEMANTIC_INVVIEW);
    this->viewProjVar = this->cameraBuffer->GetVariableByName(NEBULA_SEMANTIC_VIEWPROJECTION);
    this->invViewProjVar = this->cameraBuffer->GetVariableByName(NEBULA_SEMANTIC_INVVIEWPROJECTION);
    this->projVar = this->cameraBuffer->GetVariableByName(NEBULA_SEMANTIC_PROJECTION);
    this->invProjVar = this->cameraBuffer->GetVariableByName(NEBULA_SEMANTIC_INVPROJECTION);
    this->eyePosVar = this->cameraBuffer->GetVariableByName(NEBULA_SEMANTIC_EYEPOS);
    this->focalLengthVar = this->cameraBuffer->GetVariableByName(NEBULA_SEMANTIC_FOCALLENGTH);
    this->timeAndRandomVar = this->cameraBuffer->GetVariableByName(NEBULA_SEMANTIC_TIMEANDRANDOM);
    this->cameraBlockVar = this->sharedShader->GetVariableByName("CameraBlock");
    this->cameraBlockVar->SetBufferHandle(this->cameraBuffer->GetHandle());

    // setup shadow block, make it synced so that we can update shadow maps without massive frame drops
    this->shadowCameraBuffer = ConstantBuffer::Create();
	this->shadowCameraBuffer->SetSync(true);
    this->shadowCameraBuffer->SetupFromBlockInShader(this->sharedShader, "ShadowCameraBlock");
    this->viewMatricesVar = this->shadowCameraBuffer->GetVariableByName(NEBULA_SEMANTIC_VIEWMATRIXARRAY);
	this->shadowCameraBlockVar = this->sharedShader->GetVariableByName("ShadowCameraBlock");
    this->shadowCameraBlockVar->SetBufferHandle(this->shadowCameraBuffer->GetHandle());

    return TransformDeviceBase::Open();
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4TransformDevice::Close()
{
    this->cameraBuffer->Discard();
    this->cameraBuffer = 0;
	this->cameraBlockVar->SetBufferHandle(NULL);
    this->cameraBlockVar = 0;

    this->shadowCameraBuffer->Discard();
    this->shadowCameraBuffer = 0;
	this->shadowCameraBlockVar->SetBufferHandle(NULL);
    this->shadowCameraBlockVar = 0;

    this->viewVar = 0;
    this->invViewVar = 0;
    this->viewProjVar = 0;
    this->invViewProjVar = 0;
    this->projVar = 0;
    this->invProjVar = 0;
    this->eyePosVar = 0;
    this->focalLengthVar = 0;
	this->sharedShader = 0;

    TransformDeviceBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4TransformDevice::ApplyViewSettings()
{
    TransformDeviceBase::ApplyViewSettings();

    // update per frame view stuff
    this->cameraBuffer->CycleBuffers();
	this->cameraBuffer->BeginUpdateSync();
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
	this->cameraBuffer->EndUpdateSync();
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4TransformDevice::ApplyViewMatrixArray(const Math::matrix44* matrices, SizeT num)
{
    this->shadowCameraBuffer->CycleBuffers();
	this->shadowCameraBuffer->BeginUpdateSync();
	this->viewMatricesVar->SetMatrixArray(matrices, num);
	this->shadowCameraBuffer->EndUpdateSync();
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4TransformDevice::ApplyModelTransforms(const Ptr<Shader>& shdInst)
{    
    if (shdInst->HasVariableByName(NEBULA_SEMANTIC_MODEL))
    {
        shdInst->GetVariableByName(NEBULA_SEMANTIC_MODEL)->SetMatrix(this->GetModelTransform());
    }
    if (shdInst->HasVariableByName(NEBULA_SEMANTIC_INVMODEL))
    {
        shdInst->GetVariableByName(NEBULA_SEMANTIC_INVMODEL)->SetMatrix(this->GetInvModelTransform());
    }
}

} // namespace OpenGL4
