#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11MultipleRenderTarget
    
    Direct3D11 implementation of multiple render target.
    
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/multiplerendertargetbase.h"
namespace Direct3D11
{
class D3D11MultipleRenderTarget : public Base::MultipleRenderTargetBase
{
	__DeclareClass(D3D11MultipleRenderTarget);
public:
	/// constructor
	D3D11MultipleRenderTarget();
	/// destructor
	virtual ~D3D11MultipleRenderTarget();

	/// begins pass
	void BeginPass();
	/// ends pass
	void EndPass();

	/// begin a batch
	void BeginBatch(CoreGraphics::BatchType::Code batchType);
	/// end current batch
	void EndBatch();
}; 
} // namespace Direct3D11
//------------------------------------------------------------------------------