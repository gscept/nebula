//------------------------------------------------------------------------------
//  d3d11streamshaderloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "stdneb.h"
#include "coregraphics/d3d11/d3d11streamshaderloader.h"
#include "coregraphics/d3d11/d3d11shader.h"
#include "coregraphics/d3d11/d3d11renderdevice.h"
#include "coregraphics/d3d11/d3d11shaderserver.h"
#include "io/ioserver.h"
#include <D3DCompiler.h>
#include <d3dx11effect.h>

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11StreamShaderLoader, 'D1SL', Resources::StreamResourceLoader);

using namespace Resources;
using namespace CoreGraphics;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
bool
D3D11StreamShaderLoader::CanLoadAsync() const
{
    // no asynchronous loading supported for shader
    return false;
}

//------------------------------------------------------------------------------
/**
    Loads a precompiled shader files from a stream into 2-5 shader programs
    object.
*/
bool
D3D11StreamShaderLoader::SetupResourceFromStream(const Ptr<Stream>& stream)
{
    n_assert(stream.isvalid());
    n_assert(stream->CanBeMapped());
    ID3D11DeviceContext* context = D3D11RenderDevice::Instance()->GetDirect3DDeviceContext();
	ID3D11Device* device = D3D11RenderDevice::Instance()->GetDirect3DDevice();
    n_assert(0 != device);
	n_assert(0 != context);
    n_assert(this->resource->IsA(D3D11Shader::RTTI));
    const Ptr<D3D11Shader>& res = this->resource.downcast<D3D11Shader>();
    n_assert(!res->IsLoaded());
    
    // map stream to memory
    stream->SetAccessMode(Stream::ReadAccess);
    if (stream->Open())
    {		
		// map data
		void* data = stream->Map();
		SizeT size = stream->GetSize();

		ID3DX11Effect* effect;
		HRESULT hr = D3DX11CreateEffectFromMemory(data, size, 0, device, &effect);

		// unmap stream
		stream->Unmap();

		if (FAILED(hr))
		{
			n_error("D3D11StreamShaderLoader: failed to load shader '%s'!", 
				res->GetResourceId().Value());
			return false;
		}

		res->effect = effect;
		res->shaderName = res->GetResourceId().AsString();

        return true;
    }
    return false;
}


} // namespace Direct3D11
