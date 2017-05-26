#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11Texture
    
    D3D11/Xbox360 implementation of Texture class

    FIXME: need to handle DeviceLost through RenderDevice event handler
    (Win32 only)
    
    (C) 2007 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/texturebase.h"

namespace Direct3D11
{
class D3D11Texture : public Base::TextureBase
{
    __DeclareClass(D3D11Texture);
public:
    /// constructor
    D3D11Texture();
    /// destructor
    virtual ~D3D11Texture();

    /// unload the resource, or cancel the pending load
    virtual void Unload();
    /// map a texture mip level for CPU access
    bool Map(IndexT mipLevel, MapType mapType, MapInfo& outMapInfo);
    /// unmap texture after CPU access
    void Unmap(IndexT mipLevel);
    /// map a cube map face for CPU access
    bool MapCubeFace(CubeFace face, IndexT mipLevel, MapType mapType, MapInfo& outMapInfo);
    /// unmap cube map face after CPU access
    void UnmapCubeFace(CubeFace face, IndexT mipLevel);
	/// generates mipmaps
	void GenerateMipmaps();

	/// updates unmappable d3d11 texture
	void Update(void* data, SizeT size, SizeT width, SizeT height, const Math::rectangle<int>& region, IndexT mip);
	
    /// get d3d11 base texture pointer
    ID3D11Resource* GetD3D11BaseTexture() const;
	/// get the d3d11 texture readable by the cpu
	ID3D11Texture2D* GetCPUTexture() const;
    /// get d3d11 texture pointer
    ID3D11Texture2D* GetD3D11Texture() const;
    /// get d3d11 cube texture pointer
    ID3D11Texture2D* GetD3D11CubeTexture() const;
    /// get d3d11 volume texture pointer
    ID3D11Texture3D* GetD3D11VolumeTexture() const;
	/// gets the shader resource for the texture
	ID3D11ShaderResourceView* GetShaderResource() const;

    /// setup from a ID3D11Texture2D
    void SetupFromD3D11Texture(ID3D11Resource* ptr, const bool setLoaded = true, const bool cpuRead = true);
    /// setup from a ID3D11Texture2D //Handle array creation of cube map
    void SetupFromD3D11CubeTexture(ID3D11Resource* ptr, const bool setLoaded = true);
    /// setup from a ID3D11Texture3D
    void SetupFromD3D11VolumeTexture(ID3D11Resource* ptr, const bool setLoaded = true);

protected:
    ID3D11Resource* d3d11BaseTexture;
	ID3D11Texture2D* d3d11CPUTexture; // texture used for CPU access
    int mapCount;
    #if __WIN32__
    ID3D11Texture2D* d3d11Texture;                 // valid if type is Texture2D
	ID3D11Texture2D* d3d11CubeTexture[6];         // valid if type is TextureCube
    ID3D11Texture3D* d3d11VolumeTexture;     // valid if type is Texture3D
	ID3D11ShaderResourceView* d3d11ShaderResource;

    #endif
};

//------------------------------------------------------------------------------
/**
*/
inline ID3D11Resource* 
D3D11Texture::GetD3D11BaseTexture() const
{
    n_assert(0 != this->d3d11BaseTexture);
    n_assert(0 == this->mapCount);
    return this->d3d11BaseTexture;
}


//------------------------------------------------------------------------------
/**
*/
inline ID3D11Texture2D*
D3D11Texture::GetD3D11Texture() const
{
	return this->d3d11Texture;
}

//------------------------------------------------------------------------------
/**
*/
inline ID3D11Texture2D*
D3D11Texture::GetD3D11CubeTexture() const
{
	return this->d3d11CubeTexture[0];
}

//------------------------------------------------------------------------------
/**
*/
inline ID3D11Texture3D*
D3D11Texture::GetD3D11VolumeTexture() const
{
	return this->d3d11VolumeTexture;
} 

}// namespace Direct3D11
//------------------------------------------------------------------------------
    