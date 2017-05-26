#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11DepthStencilTarget
    
    Direct3D 11 depth-stencil target implementation.
    
    (C) 2013 Gustav Sterbrant
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/depthstenciltargetbase.h"
namespace Direct3D11
{
class D3D11DepthStencilTarget : public Base::DepthStencilTargetBase
{
	__DeclareClass(D3D11DepthStencilTarget);
public:
	/// constructor
	D3D11DepthStencilTarget();
	/// destructor
	virtual ~D3D11DepthStencilTarget();

	/// setup depth-stencil target
	void Setup();
	/// discard depth-stencil target
	void Discard();

	/// clear depth-stencil target
	void Clear();

	/// handle display resizing
	void OnDisplayResized();

	/// begin pass
	void BeginPass();
	/// end pass
	void EndPass();

	/// return pointer to depth stencil view
	ID3D11DepthStencilView* GetD3D11DepthStencilView() const;

private:
	ID3D11Texture2D* d3d11DepthStencil;	
	ID3D11DepthStencilView* d3d11DepthStencilView;
}; 


//------------------------------------------------------------------------------
/**
*/
inline ID3D11DepthStencilView* 
D3D11DepthStencilTarget::GetD3D11DepthStencilView() const
{
	return this->d3d11DepthStencilView;
}
} // namespace Direct3D11
//------------------------------------------------------------------------------