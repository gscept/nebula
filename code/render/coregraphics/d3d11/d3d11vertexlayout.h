#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11VertexLayout
  
    D3D11/Xbox360-implementation of vertex layout.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/    
//------------------------------------------------------------------------------
#include "coregraphics/base/vertexlayoutbase.h"
#include "coregraphics/shaderstate.h"

namespace Direct3D11
{
class D3D11VertexLayout : public Base::VertexLayoutBase
{
    __DeclareClass(D3D11VertexLayout);
public:
    /// constructor
    D3D11VertexLayout();
    /// destructor
    virtual ~D3D11VertexLayout();

    /// setup the vertex layout
    void Setup(const Util::Array<CoreGraphics::VertexComponent>& c);
	
    /// discard the vertex layout object
    void Discard();
    /// get pointer to d3d11 vertex declaration
    ID3D11InputLayout* GetD3D11VertexDeclaration();
        
private:

	/// create the vertex layout
	void CreateVertexLayout(const Ptr<CoreGraphics::ShaderInstance>& inst);
	
	IndexT compIndex;
	static const SizeT maxElements = 32;
	D3D11_INPUT_ELEMENT_DESC decl[maxElements];
	Util::String semanticName[32];
	Util::Dictionary<Ptr<CoreGraphics::ShaderInstance>, ID3D11InputLayout* > inputLayouts;
};



} // namespace Direct3D11
//------------------------------------------------------------------------------
