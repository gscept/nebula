#pragma once
//------------------------------------------------------------------------------
/** 
    @class Direct3D11::D3D11VertexBuffer
  
    D3D11/Xbox360 implementation of VertexBuffer.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file
*/    
//------------------------------------------------------------------------------
#include "coregraphics/base/vertexbufferbase.h"


namespace Direct3D11
{
class D3D11VertexBuffer : public Base::VertexBufferBase
{
    __DeclareClass(D3D11VertexBuffer);
public:
    /// constructor
    D3D11VertexBuffer();
    /// destructor
    virtual ~D3D11VertexBuffer();

    /// unload the resource, or cancel the pending load
    virtual void Unload();
    /// map the vertices for CPU access
    void* Map(MapType mapType);
    /// unmap the resource
    void Unmap();

    /// set d3d11 vertex buffer pointer
    void SetD3D11VertexBuffer(ID3D11Buffer* ptr);
    /// get pointer to d3d11 vertex buffer object
    ID3D11Buffer* GetD3D11VertexBuffer() const;

private:
	ID3D11Buffer* d3d11CPUVertexBuffer;
    ID3D11Buffer* d3d11VertexBuffer;
	D3D11_MAPPED_SUBRESOURCE subres;

    int mapCount;
};



//------------------------------------------------------------------------------
/**
*/
inline ID3D11Buffer*
D3D11VertexBuffer::GetD3D11VertexBuffer() const
{
    n_assert(0 != this->d3d11VertexBuffer);
    n_assert(0 == this->mapCount);
    return this->d3d11VertexBuffer;
}

} // namespace Direct3D11
//------------------------------------------------------------------------------

