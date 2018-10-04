#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11IndexBuffer
  
    D3D11/Xbox360 implementation of index buffer.
    
    FIXME: need to handle DeviceLost render event!
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/indexbufferbase.h"

namespace Direct3D11
{
class D3D11IndexBuffer : public Base::IndexBufferBase
{
    __DeclareClass(D3D11IndexBuffer);
public:
    /// constructor
    D3D11IndexBuffer();
    /// destructor
    virtual ~D3D11IndexBuffer();
    
    /// unload the resource, or cancel the pending load
    virtual void Unload();
    /// map index buffer for CPU access
    void* Map(MapType mapType);
    /// unmap the resource
    void Unmap();

    /// set d3d11 index buffer pointer
    void SetD3D11IndexBuffer(ID3D11Buffer* ptr);
    /// get d3d11 index buffer pointer
    ID3D11Buffer* GetD3D11IndexBuffer() const;

	/// get pointer to the raw data
	void* GetRawBuffer();
	/// get raw buffer size
	UINT GetRawBufferSize();
	/// set the pointer to the raw data
	void SetRawBuffer( void* data );
	/// set the size of the raw buffer
	void SetRawBufferSize( UINT size );


private:
	ID3D11Buffer* d3d11CPUIndexBuffer;
    ID3D11Buffer* d3d11IndexBuffer;
	D3D11_MAPPED_SUBRESOURCE subres;
    int mapCount;
};



//------------------------------------------------------------------------------
/**
*/
inline ID3D11Buffer*
D3D11IndexBuffer::GetD3D11IndexBuffer() const
{
    n_assert(0 != this->d3d11IndexBuffer);
    n_assert(0 == this->mapCount);
    return this->d3d11IndexBuffer;
}

} // namespace Direct3D11
//------------------------------------------------------------------------------
