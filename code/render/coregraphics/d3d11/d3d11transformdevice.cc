//------------------------------------------------------------------------------
//  d3d11transformdevice.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/d3d11/d3d11transformdevice.h"
#include "coregraphics/displaydevice.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/shadersemantics.h"
#include "coregraphics/d3d11/d3d11renderdevice.h"

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11TransformDevice, 'D1TD', Base::TransformDeviceBase);
__ImplementSingleton(Direct3D11::D3D11TransformDevice);

using namespace Util;
using namespace CoreGraphics;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
D3D11TransformDevice::D3D11TransformDevice() :
    mvpSemantic(NEBULA_SEMANTIC_MODELVIEWPROJECTION),
    modelSemantic(NEBULA_SEMANTIC_MODEL),
    viewSemantic(NEBULA_SEMANTIC_VIEW),
    modelViewSemantic(NEBULA_SEMANTIC_MODELVIEW),
	invModelSemantic(NEBULA_SEMANTIC_INVMODEL),
    invModelViewSemantic(NEBULA_SEMANTIC_INVMODELVIEW),
    invViewSemantic(NEBULA_SEMANTIC_INVVIEW),
    viewProjSemantic(NEBULA_SEMANTIC_VIEWPROJECTION),
	invViewProjSemantic(NEBULA_SEMANTIC_INVVIEWPROJECTION),
    eyePosSemantic(NEBULA_SEMANTIC_EYEPOS),
    projSemantic(NEBULA_SEMANTIC_PROJECTION),
    invProjectionSemantic(NEBULA_SEMANTIC_INVPROJECTION),
	csmSplitMatricesSemantic(NEBULA_SEMANTIC_CSMSPLITMATRICES)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
D3D11TransformDevice::~D3D11TransformDevice()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
D3D11TransformDevice::Open()
{
    ShaderServer* shdServer = ShaderServer::Instance();
    return TransformDeviceBase::Open();
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11TransformDevice::Close()
{
    TransformDeviceBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11TransformDevice::ApplyViewSettings()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11TransformDevice::ApplyModelTransforms(const Ptr<ShaderInstance>& shdInst)
{    

	// apply model view projection 
	if (shdInst->HasVariableBySemantic(this->mvpSemantic))
	{
		const Ptr<ShaderVariable>& modelViewProjMatrix = shdInst->GetVariableBySemantic(mvpSemantic);       
		modelViewProjMatrix->SetMatrix(this->GetModelViewProjTransform());
	}

	if (shdInst->HasVariableBySemantic(this->eyePosSemantic))
	{
		const Ptr<ShaderVariable>& eyePos = shdInst->GetVariableBySemantic(this->eyePosSemantic);
		eyePos->SetFloat4(this->GetInvViewTransform().getrow3());
	}
	
    // apply optional view projection
    if (shdInst->HasVariableBySemantic(this->viewProjSemantic))
    {
        const Ptr<ShaderVariable>& viewProj = shdInst->GetVariableBySemantic(this->viewProjSemantic);
        viewProj->SetMatrix(this->GetViewProjTransform());
    }

	if (shdInst->HasVariableBySemantic(this->viewSemantic))
	{
		const Ptr<ShaderVariable>& view = shdInst->GetVariableBySemantic(this->viewSemantic);
		view->SetMatrix(this->GetViewTransform());
	}

	if (shdInst->HasVariableBySemantic(this->projSemantic))
	{
		const Ptr<ShaderVariable>& proj = shdInst->GetVariableBySemantic(this->projSemantic);
		proj->SetMatrix(this->GetProjTransform());
	}

	// apply optional inversed view projection
	if (shdInst->HasVariableByName(this->invViewProjSemantic))
	{
		const Ptr<ShaderVariable>& invViewProj = shdInst->GetVariableBySemantic(invViewProjSemantic);
		invViewProj->SetMatrix(matrix44::inverse(this->GetViewProjTransform()));
	}

	// apply optional Model matrix to shader
    if (shdInst->HasVariableBySemantic(this->modelSemantic))
    {
        const Ptr<ShaderVariable>& model = shdInst->GetVariableBySemantic(this->modelSemantic);
        model->SetMatrix(this->GetModelTransform());
    }      

    // apply optional ModelView matrix to shader
    if (shdInst->HasVariableBySemantic(this->modelViewSemantic))
    {
        const Ptr<ShaderVariable>& modelView = shdInst->GetVariableBySemantic(this->modelViewSemantic);
        modelView->SetMatrix(this->GetModelViewTransform());
    }

    // apply optional InvModelView matrix to shader
    if (shdInst->HasVariableBySemantic(this->invModelViewSemantic))
    {
        const Ptr<ShaderVariable>& invModelView = shdInst->GetVariableBySemantic(this->invModelViewSemantic);
        invModelView->SetMatrix(this->GetInvModelViewTransform());
    }

	// apply optional inversed view matrix to shader
	if (shdInst->HasVariableBySemantic(this->invModelSemantic))
	{
		const Ptr<ShaderVariable>& invModel = shdInst->GetVariableBySemantic(this->invModelSemantic);
		invModel->SetMatrix(this->GetInvModelTransform());
	}

	// apply optional inversed view matrix to shader
	if (shdInst->HasVariableBySemantic(this->invViewSemantic))
	{
		const Ptr<ShaderVariable>& invView = shdInst->GetVariableBySemantic(this->invViewSemantic);
		invView->SetMatrix(this->GetInvViewTransform());
	}

    // apply optional inverse projection matrix to shader
    if (shdInst->HasVariableBySemantic(this->invProjectionSemantic))
    {
        const Ptr<ShaderVariable>& invProj = shdInst->GetVariableBySemantic(this->invProjectionSemantic);
        invProj->SetMatrix(this->GetInvProjTransform());
    }

	// apply optional CSM array
	if (shdInst->HasVariableBySemantic(this->csmSplitMatricesSemantic))
	{
		const Ptr<ShaderVariable>& splits = shdInst->GetVariableBySemantic(this->csmSplitMatricesSemantic);
		splits->SetMatrixArray(this->csmSplitMatrices, Lighting::CSMUtil::NumCascades);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11TransformDevice::SetCSMSplitMatrices( const Math::matrix44* matrices, SizeT count )
{
	n_assert(count <= Lighting::CSMUtil::NumCascades);
	IndexT i;
	for (i = 0; i < count; i++)
	{
		this->csmSplitMatrices[i] = matrices[i];
	}
}

} // namespace Direct3D11
