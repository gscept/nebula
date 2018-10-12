//------------------------------------------------------------------------------
//  d3d11memorytextureloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "stdneb.h"
#include "coregraphics/pixelformat.h"
#include "coregraphics/streamtextureloader.h"
#include "coregraphics/texture.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/memorytexturepool.h"

#include "D3D11.h"
#include "coregraphics/d3d11/d3d11types.h"

using namespace Direct3D11;
using namespace Resources;
using namespace CoreGraphics;

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11MemoryTextureLoader, 'D1MT', Resources::ResourceMemoryPool);

//------------------------------------------------------------------------------
/**
*/
void 
D3D11MemoryTextureLoader::SetImageBuffer(const void* buffer, SizeT width, SizeT height, CoreGraphics::PixelFormat::Code format)
{
	HRESULT hr;
	ID3D11Device* d3d11Device = RenderDevice::Instance()->GetDirect3DDevice();
	n_assert(0 != d3d11Device);

	d3d11Texture = 0;
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
	desc.Width = width;
	desc.ArraySize = 1;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.CPUAccessFlags = 0;
	desc.Format = D3D11Types::AsD3D11PixelFormat(format);
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;		
	D3D11_SUBRESOURCE_DATA subdata;
	ZeroMemory(&subdata,sizeof(D3D11_SUBRESOURCE_DATA));
	subdata.pSysMem = buffer;
	subdata.SysMemPitch = CoreGraphics::PixelFormat::ToSize(format)*width;
	hr = d3d11Device->CreateTexture2D(&desc,&subdata,&d3d11Texture);
	
	if (FAILED(hr))
	{
		n_error("MemoryTextureLoader: CreateTexture2D() failed");
		return;
	}

}

//------------------------------------------------------------------------------
/**
*/
bool 
D3D11MemoryTextureLoader::OnLoadRequested()
{
	n_assert(this->resource->IsA(Texture::RTTI));
	n_assert(this->d3d11Texture != NULL);
	const Ptr<Texture>& res = this->resource.downcast<Texture>();
	n_assert(!res->IsLoaded());
	res->SetupFromD3D11Texture(d3d11Texture);
	res->SetState(Resource::Loaded);
	this->SetState(Resource::Loaded);
	return true;		
}
}

