#pragma once
//------------------------------------------------------------------------------
/**
    @class Instancing::D3D11InstanceRenderer
    
    Implements a DirectX 11 instance renderer.
    
    (C) 2012 Gustav Sterbrant
*/
//------------------------------------------------------------------------------
#include "instancing/base/instancerendererbase.h"

namespace Instancing
{
class D3D11InstanceRenderer : public InstanceRendererBase
{
	__DeclareClass(D3D11InstanceRenderer);
public:
	/// constructor
	D3D11InstanceRenderer();
	/// destructor
	virtual ~D3D11InstanceRenderer();

	/// render
	void Render();

private:
	CoreGraphics::ShaderVariable::Semantic modelArraySemantic;
	CoreGraphics::ShaderVariable::Semantic modelViewArraySemantic;
	CoreGraphics::ShaderVariable::Semantic modelViewProjectionArraySemantic;

	static const int MaxInstancesPerBatch = 256;
}; 

} // namespace Instancing
//------------------------------------------------------------------------------