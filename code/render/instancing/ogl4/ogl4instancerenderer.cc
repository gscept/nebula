//------------------------------------------------------------------------------
//  ogl4instancerenderer.cc
//  (C) 2013 gscept
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "ogl4instancerenderer.h"
#include "coregraphics/ogl4/ogl4renderdevice.h"
#include "coregraphics/shadersemantics.h"
#include "coregraphics/shaderserver.h"

using namespace Math;
using namespace OpenGL4;
using namespace CoreGraphics;
namespace Instancing
{
__ImplementClass(Instancing::OGL4InstanceRenderer, 'O4IR', Instancing::InstanceRendererBase);

//------------------------------------------------------------------------------
/**
*/
OGL4InstanceRenderer::OGL4InstanceRenderer()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
OGL4InstanceRenderer::~OGL4InstanceRenderer()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4InstanceRenderer::Setup()
{
	InstanceRendererBase::Setup();

	Ptr<CoreGraphics::Shader> sharedShader = CoreGraphics::ShaderServer::Instance()->GetSharedShader();
	this->instancingBuffer = ConstantBuffer::Create();
	this->instancingBuffer->SetupFromBlockInShader(sharedShader, "InstanceBlock", 16);
	this->instancingBlockVar = sharedShader->GetVariableByName("InstanceBlock");
	this->instancingBlockVar->SetBufferHandle(this->instancingBuffer->GetHandle());

	this->modelArrayVar = this->instancingBuffer->GetVariableByName("ModelArray");
	//this->modelViewArrayVar = this->instancingBuffer->GetVariableByName("ModelViewArray");
	//this->modelViewProjectionArrayVar = this->instancingBuffer->GetVariableByName("ModelViewProjectionArray");
	this->idArrayVar = this->instancingBuffer->GetVariableByName("IdArray");
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4InstanceRenderer::Close()
{
	this->modelArrayVar = 0;
	//this->modelViewArrayVar = 0;
	//this->modelViewProjectionArrayVar = 0;
	this->idArrayVar = 0;
	this->instancingBlockVar = 0;
	this->instancingBuffer->Discard();
	this->instancingBuffer = 0;

	InstanceRendererBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4InstanceRenderer::Render(const SizeT multiplier)
{
	n_assert(this->shader.isvalid());

	// get render device
	Ptr<OGL4RenderDevice> renderDev = OGL4RenderDevice::Instance();

	// get pointer to matrix array
	matrix44* modelTrans = &this->modelTransforms[0];
	matrix44* modelViewTrans = &this->modelViewTransforms[0];
	matrix44* modelViewProjTrans = &this->modelViewProjectionTransforms[0];
    int* objectIdArray = &this->objectIds[0];

	// we assume all arrays are equally big
	SizeT instances = this->modelTransforms.Size();
	while (instances > 0)
	{
		// calculate how many transforms we will set in this batch
		int numBatchInstances = n_min(instances, this->MaxInstancesPerBatch);

		// apply variables
		this->instancingBuffer->CycleBuffers();
		this->instancingBuffer->BeginUpdateSync();
		this->modelArrayVar->SetMatrixArray(modelTrans, numBatchInstances);
		//this->modelViewArrayVar->SetMatrixArray(modelViewTrans, numBatchInstances);
		//this->modelViewProjectionArrayVar->SetMatrixArray(modelViewProjTrans, numBatchInstances);
		this->idArrayVar->SetIntArray(objectIdArray, numBatchInstances);
		this->instancingBuffer->EndUpdateSync();

		// commit shader, if it contains the instancing block (which requires shared.fxh) it will be updated here
		this->shader->Commit();

		// render!
		renderDev->DrawIndexedInstanced(numBatchInstances * multiplier, 0);

		// decrease transform count
		instances -= this->MaxInstancesPerBatch;

		// offset all arrays
		modelTrans += this->MaxInstancesPerBatch;
		modelViewTrans += this->MaxInstancesPerBatch;
		modelViewProjTrans += this->MaxInstancesPerBatch;
	}
}
} // namespace Instancing
