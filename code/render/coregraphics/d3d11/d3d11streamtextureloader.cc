//------------------------------------------------------------------------------
//  d3d11streamtextureloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------


#include "stdneb.h"
#include "coregraphics/d3d11/D3D11StreamTextureLoader.h"
#include "coregraphics/texture.h"
#include "coregraphics/renderdevice.h"
#include "io/ioserver.h"
#include "coregraphics/d3d11/d3d11types.h"

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11StreamTextureLoader, 'D1TL', Resources::StreamResourceLoader);

using namespace CoreGraphics;
using namespace Resources;
using namespace IO;

//------------------------------------------------------------------------------
/**
    This method actually setups the Texture object from the data in the stream.
*/
bool
D3D11StreamTextureLoader::SetupResourceFromStream(const Ptr<Stream>& stream)
{
    n_assert(stream.isvalid());
    n_assert(stream->CanBeMapped());
    HRESULT hr;
    ID3D11Device* d3d11Device = RenderDevice::Instance()->GetDirect3DDevice();
    n_assert(0 != d3d11Device);
    n_assert(this->resource->IsA(Texture::RTTI));
    const Ptr<Texture>& res = this->resource.downcast<Texture>();
    n_assert(!res->IsLoaded());

    stream->SetAccessMode(Stream::ReadAccess);
    if (stream->Open())
    {
        void* srcData = stream->Map();
        UINT srcDataSize = stream->GetSize();

        // first need to check image info whether to determine texture type
		D3DX11_IMAGE_INFO imageInfo;
		ID3D11Texture2D* texture = 0;
		
		hr = D3DX11GetImageInfoFromMemory(srcData, srcDataSize, NULL, &imageInfo, &hr);
        if (FAILED(hr))
        {
            n_error("D3D11StreamTextureLoader: failed to obtain image info from file '%s'!\n", this->resource->GetResourceId().Value());
            return false;
        }

        // load texture based on texture type
        if (D3D11_RESOURCE_DIMENSION_TEXTURE2D == imageInfo.ResourceDimension && imageInfo.ArraySize == 1)
        {
            // mipmap usage test enabled? -> DEBUG ONLY!
            Ptr<Stream> mipMapStream;
            bool visualizeMipMaps = RenderDevice::Instance()->GetVisualizeMipMaps();
            if (visualizeMipMaps)
            {
                if ((imageInfo.Width == imageInfo.Height) &&
                    (imageInfo.Width >= 128) && (imageInfo.Width <= 2048) &&
                    ((imageInfo.Format == DXGI_FORMAT_BC1_UNORM ) || (imageInfo.Format == DXGI_FORMAT_BC3_UNORM )) &&
                    (!Util::String::MatchPattern(stream->GetURI().AsString(), "*dx9*")) &&
                    (!Util::String::MatchPattern(stream->GetURI().AsString(), "*lightmap*")))
                {
                    // overwrite texture data with mipmap test texture
                    Util::String mipTestFilename;
                    mipTestFilename.Format("systex:system/miptest_%d.dds", (imageInfo.Width > imageInfo.Height) ? imageInfo.Width : imageInfo.Height);
                    // create stream
                    mipMapStream = IoServer::Instance()->CreateStream(IO::URI(mipTestFilename));
                    mipMapStream->SetAccessMode(Stream::ReadAccess);
                    mipMapStream->Open();
                    // overwrite data ptr
                    srcData = mipMapStream->Map();
                }
            }
            // load 2D texture
            ID3D11Resource* d3d11Texture = 0;
			hr = D3DX11CreateTextureFromMemory(d3d11Device, srcData, srcDataSize, NULL, NULL, &d3d11Texture, 0);
            
			// DX 9 stuff
			//hr = D3DXCreateTextureFromFileInMemory(d3d11Device, srcData, srcDataSize, &d3d11Texture);
            if (FAILED(hr))
            {
                n_error("D3D11StreamTextureLoader: D3DX11CreateTextureFromMemory() failed for file '%s'!", this->resource->GetResourceId().Value());
                return false;
            }
            res->SetupFromD3D11Texture(d3d11Texture);

            if (mipMapStream.isvalid() && visualizeMipMaps)
            {
                mipMapStream->Unmap();
                mipMapStream->Close();
            }
        }
        else if (D3D11_RESOURCE_DIMENSION_TEXTURE3D == imageInfo.ResourceDimension)
        {
            // load 3D texture
            ID3D11Resource* d3d11VolumeTexture = 0;
			hr = D3DX11CreateTextureFromMemory(d3d11Device, srcData, srcDataSize, NULL, NULL, &d3d11VolumeTexture, &hr);
			
			// DX 9 stuff
			//hr = D3DXCreateVolumeTextureFromFileInMemory(d3d11Device, srcData, srcDataSize, &d3d11VolumeTexture);
            if (FAILED(hr))
            {
                n_error("D3D11StreamTextureLoader: D3DX11CreateTextureFromMemory() failed for file '%s'!", this->resource->GetResourceId().Value());
                return false;
            }
            res->SetupFromD3D11VolumeTexture(d3d11VolumeTexture);
        }
        else if (D3D11_RESOURCE_DIMENSION_TEXTURE2D == imageInfo.ResourceDimension && 6 == imageInfo.ArraySize)
        {
            // load cube texture

			// seeing as a cube texture is really just a buffer consisting of 6 textures
            ID3D11Resource* d3d11CubeTexture = 0;
			hr = D3DX11CreateTextureFromMemory(d3d11Device, srcData, srcDataSize, NULL, NULL, &d3d11CubeTexture, &hr);

			// DX 9 stuff
            //hr = D3DXCreateCubeTextureFromFileInMemory(d3d11Device, srcData, srcDataSize, &d3d11CubeTexture);
            if (FAILED(hr))
            {
                n_error("D3D11StreamTextureLoader: D3DX11CreateTextureFromMemory() failed for file '%s'!", this->resource->GetResourceId().Value());
                return false;
            }
            res->SetupFromD3D11CubeTexture(d3d11CubeTexture);
        }
        stream->Unmap();
        stream->Close();
        return true;
    }
    return false;
}

} // namespace Direct3D11
