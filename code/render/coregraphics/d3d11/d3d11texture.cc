//------------------------------------------------------------------------------
//  d3d11texture.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "stdneb.h"
#include "coregraphics/d3d11/D3D11Texture.h"
#include "coregraphics/d3d11/d3d11types.h"
#include "coregraphics/renderdevice.h"

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11Texture, 'D1TX', Base::TextureBase);

using namespace CoreGraphics;
using namespace Direct3D11;

//------------------------------------------------------------------------------
/**
*/
D3D11Texture::D3D11Texture() :
    d3d11BaseTexture(0),
    d3d11Texture(0),
	d3d11ShaderResource(0),
    d3d11VolumeTexture(0),
	d3d11CPUTexture(0),
    mapCount(0)
{
	// set cube texture to 0
	for (IndexT i = 0; i < 6; i++)
	{
		d3d11CubeTexture[i] = 0;
	}
}

//------------------------------------------------------------------------------
/**
*/
D3D11Texture::~D3D11Texture()
{
    n_assert(0 == this->d3d11BaseTexture);
    n_assert(0 == this->mapCount);
    n_assert(0 == this->d3d11Texture);
	n_assert(0 == this->d3d11CPUTexture);
    n_assert(0 == this->d3d11CubeTexture[0]);
    n_assert(0 == this->d3d11VolumeTexture);
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11Texture::Unload()
{
    n_assert(0 == this->mapCount);
    if (0 != this->d3d11BaseTexture)
    {
        this->d3d11BaseTexture->Release();
        this->d3d11BaseTexture = 0;
    }
    if (0 != this->d3d11Texture)
    {
        this->d3d11Texture->Release();
        this->d3d11Texture = 0;
    }

	if (0 != this->d3d11CPUTexture)
	{
		this->d3d11CPUTexture->Release();
		this->d3d11CPUTexture = 0;
	}

	// release shader resource
	if (this->d3d11ShaderResource)
	{
		this->d3d11ShaderResource->Release();
		this->d3d11ShaderResource = 0;
	}	

	for (uint i = 0; i < 6; i++)
	{
		if (0 != this->d3d11CubeTexture[i])
		{
			this->d3d11CubeTexture[i]->Release();
			this->d3d11CubeTexture[i] = 0;
		}
	}        
    if (0 != this->d3d11VolumeTexture)
    {
        this->d3d11VolumeTexture->Release();
        this->d3d11VolumeTexture = 0;
    }
    TextureBase::Unload();
}

//------------------------------------------------------------------------------
/**
*/
bool
D3D11Texture::Map(IndexT mipLevel, MapType mapType, MapInfo& outMapInfo)
{
    n_assert((this->type == Texture2D) || (this->type == Texture3D));
    n_assert(MapWriteNoOverwrite != mapType);
    bool retval = false;
    UINT mapFlags = 0;
    switch (mapType)
    {
        case MapRead:
            n_assert(AccessRead == this->access);
            mapFlags |= D3D11_MAP_READ;
            break;
        case MapWrite:
            n_assert((UsageDynamic == this->usage) && (AccessWrite == this->access));
			mapFlags |= D3D11_MAP_WRITE;
            break;
        case MapReadWrite:
            n_assert((UsageDynamic == this->usage) && (AccessReadWrite == this->access));
			mapFlags |= D3D11_MAP_READ_WRITE;
            break;
        case MapWriteDiscard:
            n_assert((UsageDynamic == this->usage) && (AccessWrite == this->access));
            mapFlags |= D3D11_MAP_WRITE_DISCARD;
            break;
    }

	ID3D11DeviceContext* context = D3D11RenderDevice::Instance()->GetDirect3DDeviceContext();
	D3D11_MAPPED_SUBRESOURCE subres;
    if (Texture2D == this->type)
    {
		HRESULT hr = context->Map(this->d3d11CPUTexture, mipLevel, (D3D11_MAP)mapFlags, 0, &subres);
        if (SUCCEEDED(hr))
        {
            outMapInfo.data = subres.pData;
            outMapInfo.rowPitch = subres.RowPitch;
            outMapInfo.depthPitch = 0;
            retval = true;
        }
    }
    else if (Texture3D == this->type)
    {
		HRESULT hr = context->Map(this->d3d11VolumeTexture, mipLevel, (D3D11_MAP)mapFlags, 0, &subres);
        if (SUCCEEDED(hr))
        {
            outMapInfo.data = subres.pData;
            outMapInfo.rowPitch = subres.RowPitch;
            outMapInfo.depthPitch = subres.DepthPitch;
            retval = true;
        }
    }
    if (retval)
    {
        this->mapCount++;
    }
    return retval;
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11Texture::Unmap(IndexT mipLevel)
{
    n_assert(this->mapCount > 0);
    n_assert((Texture2D == this->type) || (Texture3D == this->type));
    if (Texture2D == this->type)
    {
		ID3D11DeviceContext* context = D3D11RenderDevice::Instance()->GetDirect3DDeviceContext();
		context->Unmap(this->d3d11CPUTexture, mipLevel);
    }
    else if (Texture3D == this->type)
    {
		ID3D11DeviceContext* context = D3D11RenderDevice::Instance()->GetDirect3DDeviceContext();
		context->Unmap(this->d3d11VolumeTexture, mipLevel);        
    }
    this->mapCount--;
}

//------------------------------------------------------------------------------
/**
*/
bool
D3D11Texture::MapCubeFace(CubeFace face, IndexT mipLevel, MapType mapType, MapInfo& outMapInfo)
{
    n_assert(TextureCube == this->type);
    n_assert(MapWriteNoOverwrite != mapType);
    UINT mapFlags = 0;
    switch (mapType)
    {
        case MapRead:
            n_assert(AccessRead == this->access);
            mapFlags |= D3D11_MAP_READ;
            break;
        case MapWrite:
            n_assert((UsageDynamic == this->usage) && (AccessWrite == this->access));
			mapFlags |= D3D11_MAP_WRITE;
            break;
        case MapReadWrite:
            n_assert((UsageDynamic == this->usage) && (AccessReadWrite == this->access));
			mapFlags |= D3D11_MAP_READ_WRITE;
            break;
        case MapWriteDiscard:
            n_assert((UsageDynamic == this->usage) && (AccessWrite == this->access));
            mapFlags |= D3D11_MAP_WRITE_DISCARD;
            break;
    }
	ID3D11DeviceContext* context = D3D11RenderDevice::Instance()->GetDirect3DDeviceContext();
	D3D11_MAPPED_SUBRESOURCE subres;
	HRESULT hr = context->Map(d3d11CubeTexture[face], mipLevel, (D3D11_MAP)mapFlags, 0, &subres);
    if (SUCCEEDED(hr))
    {
        outMapInfo.data = subres.pData;
        outMapInfo.rowPitch = subres.RowPitch;
        outMapInfo.depthPitch = 0;
        this->mapCount++;
        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11Texture::UnmapCubeFace(CubeFace face, IndexT mipLevel)
{
    n_assert(TextureCube == this->type);
    n_assert(this->mapCount > 0);
	ID3D11DeviceContext* context = D3D11RenderDevice::Instance()->GetDirect3DDeviceContext();
	context->Unmap(d3d11CubeTexture[face], mipLevel);    
    this->mapCount--;
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11Texture::Update( void* data, SizeT size, SizeT width, SizeT height, const Math::rectangle<int>& region, IndexT mip )
{
	n_assert(Texture2D == this->type)
	ID3D11DeviceContext* context = D3D11RenderDevice::Instance()->GetDirect3DDeviceContext();
	D3D11_BOX box;
	if (region.width() == region.height() == 0)
	{
		context->UpdateSubresource(this->d3d11Texture, mip, NULL, data, size/width, size);
	}
	else
	{
		box.left = region.left;
		box.top = region.top;
		box.front = 0;
		box.right = region.right;
		box.bottom = region.bottom;
		box.back = 0;

		context->UpdateSubresource(this->d3d11Texture, mip, &box, data, size/width, size);
	}
	
}

//------------------------------------------------------------------------------
/**
    Helper method to setup the texture object from a D3D11 2D texture.
*/
void
D3D11Texture::SetupFromD3D11Texture(ID3D11Resource* tex2D, const bool setLoaded, const bool cpuRead)
{
    n_assert(0 != tex2D);    
    HRESULT hr = S_OK;

    // need to query for base interface under Win32
	
    this->d3d11Texture = (ID3D11Texture2D*)tex2D;

    this->SetType(D3D11Texture::Texture2D);
    
	D3D11_TEXTURE2D_DESC desc;
	this->d3d11Texture->GetDesc(&desc);

    this->SetWidth(desc.Width);
    this->SetHeight(desc.Height);
    this->SetDepth(1);
    this->SetNumMipLevels(desc.MipLevels);
    this->SetPixelFormat(D3D11Types::AsNebulaPixelFormat(desc.Format));
    if (setLoaded)
    {
        this->SetState(Resource::Loaded);
    }

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = desc.Format;
	if (desc.SampleDesc.Count > 1)
	{
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
	}
	else
	{
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	}
	viewDesc.Texture2D.MipLevels = desc.MipLevels;
	viewDesc.Texture2D.MostDetailedMip = 0;

	if ((D3D11_BIND_SHADER_RESOURCE & desc.BindFlags) != 0)
	{
		hr = D3D11RenderDevice::Instance()->GetDirect3DDevice()->CreateShaderResourceView(this->d3d11Texture, &viewDesc, &this->d3d11ShaderResource);
		n_assert(SUCCEEDED(hr));
	}

	if (cpuRead && desc.SampleDesc.Count == 1)
	{
		// create the cpu texture
		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.MiscFlags = 0;
		this->access = ResourceBase::AccessRead;

		hr = D3D11RenderDevice::Instance()->GetDirect3DDevice()->CreateTexture2D(&desc, NULL, &this->d3d11CPUTexture);
		n_assert(SUCCEEDED(hr));
	}

}

//------------------------------------------------------------------------------
/**
    Helper method to setup the texture object from a D3D11 volume texture.
*/
void
D3D11Texture::SetupFromD3D11VolumeTexture(ID3D11Resource* texVolume, const bool setLoaded)
{
    n_assert(0 != texVolume);
    HRESULT hr = S_OK;

    this->d3d11VolumeTexture = (ID3D11Texture3D*)texVolume;	

    this->SetType(D3D11Texture::Texture3D);
    D3D11_TEXTURE3D_DESC desc;
    this->d3d11VolumeTexture->GetDesc(&desc);

    this->SetWidth(desc.Width);
    this->SetHeight(desc.Height);
    this->SetDepth(desc.Depth);
    this->SetNumMipLevels(desc.MipLevels);
    this->SetPixelFormat(D3D11Types::AsNebulaPixelFormat(desc.Format));
    if (setLoaded)
    {
        this->SetState(Resource::Loaded);
    }

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	D3D11_TEX3D_SRV srv;
	srv.MipLevels = desc.MipLevels;
	srv.MostDetailedMip = 0;
	viewDesc.Format = desc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	viewDesc.Texture3D = srv;

	if (D3D11_BIND_SHADER_RESOURCE == desc.BindFlags)
	{
		hr = D3D11RenderDevice::Instance()->GetDirect3DDevice()->CreateShaderResourceView(this->d3d11VolumeTexture, &viewDesc, &this->d3d11ShaderResource);
	}

	n_assert(SUCCEEDED(hr));
}

//------------------------------------------------------------------------------
/**
    Helper method to setup the texture object from a D3D11 cube texture.
*/
void
D3D11Texture::SetupFromD3D11CubeTexture(ID3D11Resource* texCube, const bool setLoaded)
{
    n_assert(0 != texCube);
    HRESULT hr = S_OK;

    this->d3d11CubeTexture[0] = (ID3D11Texture2D*)texCube;

    this->SetType(D3D11Texture::TextureCube);
	D3D11_TEXTURE2D_DESC desc;
    
	// all faces of the cube map must follow the same description
    this->d3d11CubeTexture[0]->GetDesc(&desc);

    this->SetWidth(desc.Width);
    this->SetHeight(desc.Height);
    this->SetNumMipLevels(desc.MipLevels);
    this->SetPixelFormat(D3D11Types::AsNebulaPixelFormat(desc.Format));
    if (setLoaded)
    {
        this->SetState(Resource::Loaded);
    }

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	D3D11_TEXCUBE_SRV srv;
	srv.MipLevels = desc.MipLevels;
	srv.MostDetailedMip = 0;
	viewDesc.Format = desc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	viewDesc.TextureCube = srv;

	if (D3D11_BIND_SHADER_RESOURCE == desc.BindFlags)
	{
		hr = D3D11RenderDevice::Instance()->GetDirect3DDevice()->CreateShaderResourceView(this->d3d11CubeTexture[0], &viewDesc, &this->d3d11ShaderResource);
	}

	n_assert(SUCCEEDED(hr));
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11Texture::GenerateMipmaps()
{
	D3D11RenderDevice::Instance()->GetDirect3DDeviceContext()->GenerateMips(d3d11ShaderResource);
}

//------------------------------------------------------------------------------
/**
*/
ID3D11ShaderResourceView* 
D3D11Texture::GetShaderResource() const
{
	n_assert(0 != d3d11ShaderResource);
	return this->d3d11ShaderResource;
}

//------------------------------------------------------------------------------
/**
*/
ID3D11Texture2D* 
D3D11Texture::GetCPUTexture() const
{
	n_assert(0 != this->d3d11CPUTexture);
	n_assert(0 == this->mapCount);
	D3D11RenderDevice::Instance()->GetDirect3DDeviceContext()->CopyResource(this->d3d11CPUTexture, this->d3d11Texture);
    return this->d3d11CPUTexture;
}

} // namespace Direct3D11

