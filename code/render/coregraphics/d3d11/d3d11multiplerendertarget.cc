//------------------------------------------------------------------------------
//  d3d11multiplerendertarget.cc
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "d3d11multiplerendertarget.h"
#include "coregraphics/renderdevice.h"

using namespace CoreGraphics;
namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11MultipleRenderTarget, 'D1MR', Base::MultipleRenderTargetBase);

//------------------------------------------------------------------------------
/**
*/
D3D11MultipleRenderTarget::D3D11MultipleRenderTarget()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
D3D11MultipleRenderTarget::~D3D11MultipleRenderTarget()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11MultipleRenderTarget::BeginPass()
{
	n_assert(this->numRenderTargets > 0);
	D3D11RenderDevice* renderDevice = RenderDevice::Instance();

	// clear all render targets
	// avoid running BeginPass on render targets since it might invoke their own depth-stencil which may already be used by this MRT
	IndexT i;
	for (i = 0; i < this->numRenderTargets; i++)
	{
		this->renderTarget[i]->SetClearColor(this->clearColor[i]);
		this->renderTarget[i]->SetClearFlags(this->clearFlags[i]);
		this->renderTarget[i]->Clear();
		renderDevice->AddRenderTarget(this->renderTarget[i]->GetD3D11RenderTargetView());
	}

	// then begin the pass for the depth-stencil
	// this depth-stencil will take precidence over any other depth-stencil found in any render target
	if (this->depthStencilTarget.isvalid())
	{
		this->depthStencilTarget->SetClearDepth(this->clearDepth);
		this->depthStencilTarget->SetClearStencil(this->clearStencil);
		this->depthStencilTarget->SetClearFlags(this->depthStencilClearFlags);
		this->depthStencilTarget->BeginPass();
		renderDevice->SetDepthStencilTarget(this->depthStencilTarget->GetD3D11DepthStencilView());
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11MultipleRenderTarget::EndPass()
{
	if (this->depthStencilTarget.isvalid())
	{
		this->depthStencilTarget->EndPass();
	}
}	

//------------------------------------------------------------------------------
/**
*/
void 
D3D11MultipleRenderTarget::BeginBatch( CoreGraphics::BatchType::Code batchType )
{
	// empty on purpose
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11MultipleRenderTarget::EndBatch()
{
	// empty on purpose
}

} // namespace Direct3D11