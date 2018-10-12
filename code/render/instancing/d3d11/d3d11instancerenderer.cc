//------------------------------------------------------------------------------
//  d3d11instancerenderer.cc
//  (C) 2012 Gustav Sterbrant
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "d3d11instancerenderer.h"
#include "coregraphics/d3d11/d3d11renderdevice.h"
#include "coregraphics/shadersemantics.h"

using namespace Math;
using namespace Direct3D11;
using namespace CoreGraphics;
namespace Instancing
{
__ImplementClass(Instancing::D3D11InstanceRenderer, 'D1IR', Instancing::InstanceRendererBase);
//------------------------------------------------------------------------------
/**
*/
D3D11InstanceRenderer::D3D11InstanceRenderer() :
	modelArraySemantic(NEBULA_SEMANTIC_MODELARRAY),
	modelViewArraySemantic(NEBULA_SEMANTIC_MODELVIEWARRAY),
	modelViewProjectionArraySemantic(NEBULA_SEMANTIC_MODELVIEWPROJECTIONARRAY)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
D3D11InstanceRenderer::~D3D11InstanceRenderer()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11InstanceRenderer::Render()
{
	n_assert(this->shader->IsValid());

	// get render device
	Ptr<D3D11RenderDevice> renderDev = D3D11RenderDevice::Instance();

	// create shader variables
	Ptr<ShaderVariable> modelArrayVar = 0;
	Ptr<ShaderVariable> modelViewArrayVar = 0;
	Ptr<ShaderVariable> modelViewProjectionArrayVar = 0;

	// get variables
	if (this->shader->HasVariableBySemantic(this->modelArraySemantic))
	{
		modelArrayVar = this->shader->GetVariableBySemantic(this->modelArraySemantic);
	}

	if (this->shader->HasVariableBySemantic(this->modelViewArraySemantic))
	{
		modelViewArrayVar = this->shader->GetVariableBySemantic(this->modelViewArraySemantic);
	}

	if (this->shader->HasVariableBySemantic(this->modelViewProjectionArraySemantic))
	{
		modelViewProjectionArrayVar = this->shader->GetVariableBySemantic(this->modelViewProjectionArraySemantic);
	}

	// get pointer to matrix array
	matrix44* modelTrans = &this->modelTransforms[0];
	matrix44* modelViewTrans = &this->modelViewTransforms[0];
	matrix44* modelViewProjTrans = &this->modelViewProjectionTransforms[0];

	// we assume all arrays are equally big
	SizeT instances = this->modelTransforms.Size();
	while (instances > 0)
	{
		// calculate how many transforms we will set in this batch
		int numBatchInstances = min(instances, this->MaxInstancesPerBatch);

		// apply variables
		if (modelArrayVar.isvalid())
		{
			modelArrayVar->SetMatrixArray(modelTrans, numBatchInstances);
		}
		if (modelViewArrayVar.isvalid())
		{
			modelViewArrayVar->SetMatrixArray(modelViewTrans, numBatchInstances);
		}
		if (modelViewProjectionArrayVar.isvalid())
		{
			modelViewProjectionArrayVar->SetMatrixArray(modelViewProjTrans, numBatchInstances);
		}		

		// commit shader
		this->shader->Commit();

		// render!
		renderDev->DrawIndexedInstanced(numBatchInstances);

		// decrease transform count
		instances -= this->MaxInstancesPerBatch;

		// offset all arrays
		modelTrans += this->MaxInstancesPerBatch;
		modelViewTrans += this->MaxInstancesPerBatch;
		modelViewProjTrans += this->MaxInstancesPerBatch;
	}
}
}
