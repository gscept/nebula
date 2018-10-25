//------------------------------------------------------------------------------
//  d3d11streamtexturesaver.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "stdneb.h"
#include "coregraphics/d3d11/d3d11streamtexturesaver.h"
#include "coregraphics/d3d11/d3d11types.h"
#include "coregraphics/texture.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/base/texturebase.h"
#include "IL/il.h"
#include "IL/ilu.h"
#include "math/scalar.h"

#ifdef RGB
#undef RGB
#endif
//#define RGB(r,g,b) (((r & 0xff)  << 24) | ((g & 0xff) << 16) | ((b & 0xff) << 8) | 255);
//#define RGBA(r,g,b,a) ((r << 24) | (g << 16) | (b << 8) | a);
#define RGB(r,g,b) (uint)( ((255 & 0xff) << 24) |((b & 0xff) << 16) | ((g & 0xff) << 8) | ((r & 0xff)) )
#define RGBA(r,g,b,a) (uint)( ((a & 0xff) << 24) | ((b & 0xff) << 16) | ((g & 0xff) << 8) | ((r & 0xff)) )

#define RED(rgb) (rgb & 0xff)
#define BLUE(rgb) ((rgb >> 16) & 0xff)
#define GREEN(rgb) ((rgb >> 8) & 0xff)
#define ALPHA(rgb) (rgb >> 24)

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11StreamTextureSaver, 'D1TS', Base::StreamTextureSaverBase);

using namespace IO;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
bool
D3D11StreamTextureSaver::OnSave()
{
    n_assert(this->stream.isvalid());
    const Ptr<Texture>& tex = this->resource.downcast<Texture>();
    n_assert(tex->GetType() == Texture::Texture2D);
    bool retval = false;

	ID3D11DeviceContext* context = D3D11RenderDevice::Instance()->GetDirect3DDeviceContext();
	ID3D11Device* device = D3D11RenderDevice::Instance()->GetDirect3DDevice();

    SizeT maxLevels = tex->GetNumMipLevels();
    SizeT mipLevelToSave = this->mipLevel;
    if (mipLevelToSave >= maxLevels)
    {
        mipLevelToSave = maxLevels - 1;
    }

	ID3D11Texture2D* d3d11Texture = tex->GetCPUTexture();
	D3D11_TEXTURE2D_DESC desc;
	d3d11Texture->GetDesc(&desc);
	n_assert(0 != d3d11Texture);
	
	// calculate mipped sizes
	int mippedWidth = int(desc.Width / pow(2.0f, mipLevelToSave));
	int mippedHeight = int(desc.Height / pow(2.0f, mipLevelToSave));

	// create il image
	int image = ilGenImage();
	ilBindImage(image);

	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE subres;
	hr = context->Map(d3d11Texture, mipLevelToSave, D3D11_MAP_READ, NULL, &subres);
	n_assert(SUCCEEDED(hr));

	// DX automatically pads images, so we need to adjust for that...
	int pixelSize = PixelFormat::ToSize(tex->GetPixelFormat());
	int calcedPitch = mippedWidth * pixelSize;
	int pitchDiff = subres.RowPitch - calcedPitch;
	mippedWidth += pitchDiff / pixelSize;

	if (tex->GetPixelFormat() == PixelFormat::DXT1 ||
		tex->GetPixelFormat() == PixelFormat::DXT3 ||
		tex->GetPixelFormat() == PixelFormat::DXT5)
	{
		ILuint channels;
		ILuint format;
		ILenum dxtFormat;
		if (tex->GetPixelFormat() == PixelFormat::DXT1)			{ channels = 3; format = IL_RGB; dxtFormat = IL_DXT1; }
		else if (tex->GetPixelFormat() == PixelFormat::DXT3)	{ channels = 4; format = IL_RGBA; dxtFormat = IL_DXT3; }
		else if (tex->GetPixelFormat() == PixelFormat::DXT5)	{ channels = 4; format = IL_RGBA; dxtFormat = IL_DXT5; }

		// create image
		ILboolean result = ilTexImageDxtc(mippedWidth, mippedHeight, 1, dxtFormat, (ILubyte*)subres.pData);
		n_assert(result == IL_TRUE);
		ilDxtcDataToSurface();
	}
	else
	{
		ILboolean result;
		switch (tex->GetPixelFormat())
		{
		case PixelFormat::R32F:
			{
				// load image
				result = ilTexImage(mippedWidth, mippedHeight, 1, 1, IL_LUMINANCE, IL_FLOAT, subres.pData);
				ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
				iluImageParameter(ILU_PLACEMENT, ILU_UPPER_LEFT);

				// flip image
				iluFlipImage();
			}
			break;
		case PixelFormat::G32R32F:
			{
				// load image
				result = ilTexImage(mippedWidth, mippedHeight, 1, 2, IL_LUMINANCE_ALPHA, IL_FLOAT, subres.pData);
				ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
				iluImageParameter(ILU_PLACEMENT, ILU_UPPER_LEFT);

				// flip image
				iluFlipImage();
			}
			break;
		case PixelFormat::A16B16G16R16:
			{
				// load image
				result = ilTexImage(mippedWidth, mippedHeight, 1, 4, IL_RGBA, IL_UNSIGNED_SHORT, subres.pData);
				ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
				iluImageParameter(ILU_PLACEMENT, ILU_UPPER_LEFT);

				// flip image
				iluFlipImage();
			}
			break;
		case PixelFormat::A16B16G16R16F:
			{
				// load image
				result = ilTexImage(mippedWidth, mippedHeight, 1, 4, IL_RGBA, IL_HALF, subres.pData);
				ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
				iluImageParameter(ILU_PLACEMENT, ILU_UPPER_LEFT);

				// flip image
				iluFlipImage();
			}
			break;
		case PixelFormat::A32B32G32R32F:
			{
				// load image
				result = ilTexImage(mippedWidth, mippedHeight, 1, 4, IL_RGBA, IL_FLOAT, subres.pData);
				ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
				iluImageParameter(ILU_PLACEMENT, ILU_UPPER_LEFT);

				// flip image
				iluFlipImage();
			}
			break;
		case PixelFormat::X8R8G8B8:
			{
				// load image
				result = ilTexImage(mippedWidth, mippedHeight, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, subres.pData);
				ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
				iluImageParameter(ILU_PLACEMENT, ILU_UPPER_LEFT);

				// flip image
				iluFlipImage();
			}
			break;
		default:
			{
				// load image
				result = ilTexImage(mippedWidth, mippedHeight, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, subres.pData);
				ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
				iluImageParameter(ILU_PLACEMENT, ILU_UPPER_LEFT);

				// flip image
				iluFlipImage();
			}
		}
	}

	context->Unmap(d3d11Texture, mipLevelToSave);

	// now save as bmp
	ILint size = ilSaveL(IL_PNG, NULL, 0);
	ILbyte* data = new ILbyte[size];
	ilSaveL(IL_PNG, data, size);

    // write result to stream
    this->stream->SetAccessMode(Stream::WriteAccess);
    if (this->stream->Open())
    {
		this->stream->Write(data, size);
        this->stream->Close();
		this->stream->SetMediaType(ImageFileFormat::ToMediaType(this->format));
        retval = true;
    }

	// free il image
	ilDeleteImage(image);
    return retval;
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11StreamTextureSaver::DecompressDXT1( uint x, uint y, uchar* source, uint* destination, uint width )
{
	ushort color0, color1;
	color0 = *reinterpret_cast<const ushort *>(source);
	color1 = *reinterpret_cast<const ushort *>(source + 2);

	// sizeof int is two shorts
	uint code = *reinterpret_cast<const unsigned long *>(source + 4);

	uint temp;
	temp = (color0 >> 11) * 255 + 16;
	uchar r0 = (uchar)((temp/32 + temp)/32);
	temp = ((color0 & 0x07E0) >> 5) * 255 + 32;
	uchar g0 = (uchar)((temp/64 + temp)/64);
	temp = (color0 & 0x001F) * 255 + 16;
	uchar b0 = (uchar)((temp/32 + temp)/32);

	temp = (color1 >> 11) * 255 + 16;
	uchar r1 = (uchar)((temp/32 + temp)/32);
	temp = ((color1 & 0x07E0) >> 5) * 255 + 32;
	uchar g1 = (uchar)((temp/64 + temp)/64);
	temp = (color1 & 0x001F) * 255 + 16;
	uchar b1 = (uchar)((temp/32 + temp)/32);

	uint rgb0, rgb1;

	rgb0 = RGB(r0, g0, b0);
	rgb1 = RGB(r1, g1, b1);

	uint color2, color3;

	if (color0 > color1)
	{
		color2 = RGB((2*r0+r1)/3, (2*g0+g1)/3, (2*b0+b1)/3);
		color3 = RGB((r0+2*r1)/3, (g0+2*g1)/3, (b0+2*b1)/3);
	}
	else
	{
		color2 = RGB((r0+r1)/2, (g0+g1)/2, (b0+b1)/2);
		color3 = RGB(0,0,0);
	}

	uint colors[4];
	colors[0] = rgb0;
	colors[1] = rgb1;
	colors[2] = color2;
	colors[3] = color3;
	
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			uchar posCode = (code >> 2*(4*i+j)) & 0x03;
			uint color = colors[posCode];

			if (x + i < width)
			{
				destination[(y+i)*width + (x+j)] = color;
			}
		}
	}

}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11StreamTextureSaver::DecompressDXT3( uint x, uint y, uchar* source, uint* destination, uint width )
{
	ubyte alphas[8];
	alphas[0] = *reinterpret_cast<const ubyte *>(source);
	alphas[1] = *reinterpret_cast<const ubyte *>(source+1);
	alphas[2] = *reinterpret_cast<const ubyte *>(source+2);
	alphas[3] = *reinterpret_cast<const ubyte *>(source+3);
	alphas[4] = *reinterpret_cast<const ubyte *>(source+4);
	alphas[5] = *reinterpret_cast<const ubyte *>(source+5);
	alphas[6] = *reinterpret_cast<const ubyte *>(source+6);
	alphas[7] = *reinterpret_cast<const ubyte *>(source+7);

	ushort color0, color1;
	color0 = *reinterpret_cast<const ushort *>(source + 8);
	color1 = *reinterpret_cast<const ushort *>(source + 10);

	uint code = *reinterpret_cast<const uint *>(source + 12);

	uint temp;
	temp = (color0 >> 11) * 255 + 16;
	ubyte r0 = (ubyte)((temp/32 + temp)/32);
	temp = ((color0 & 0x07E0) >> 5) * 255 + 32;
	ubyte g0 = (ubyte)((temp/64 + temp)/64);
	temp = (color0 & 0x001F) * 255 + 16;
	ubyte b0 = (ubyte)((temp/32 + temp)/32);

	temp = (color1 >> 11) * 255 + 16;
	ubyte r1 = (ubyte)((temp/32 + temp)/32);
	temp = ((color1 & 0x07E0) >> 5) * 255 + 32;
	ubyte g1 = (ubyte)((temp/64 + temp)/64);
	temp = (color1 & 0x001F) * 255 + 16;
	ubyte b1 = (ubyte)((temp/32 + temp)/32);

	uint rgb0 = RGB(r0, g0, b0);
	uint rgb1 = RGB(r1, g1, b1);
	uint color2 = RGB((2 * r0 + r1)/3, (2 * g0 + g1)/3, (2 * b0 + b1)/3);
	uint color3 = RGB((r0 + 2 * r1)/3, (g0 + 2 * g1)/3, (b0 + 2 * b1)/3);	

	uint colors[4];
	colors[0] = rgb0;
	colors[1] = rgb1;
	colors[2] = color2;
	colors[3] = color3;

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			ubyte posCode = (code >> 2*(4*i+j)) & 0x03;
			uint alpha = alphas[posCode] + alphas[posCode+1] * 256;
			uint color = colors[posCode];
			uint red = RED(color);
			uint green = GREEN(color);
			uint blue = BLUE(color);

			destination[(y+i)*width + (x+j)] = RGBA(red, green, blue, alpha);
		}
	}

}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11StreamTextureSaver::DecompressDXT5( uint x, uint y, uchar* source, uint* destination, uint width )
{

	uchar alpha0 = *reinterpret_cast<const uchar *>(source);
	uchar alpha1 = *reinterpret_cast<const uchar *>(source + 1);

	const uchar *bits = source + 2;
	uint alphaCode1 = bits[2] | (bits[3] << 8) | (bits[4] << 16) | (bits[5] << 24);
	ushort alphaCode2 = bits[0] | (bits[1] << 8);

	ushort color0 = *reinterpret_cast<const ushort *>(source + 8);
	ushort color1 = *reinterpret_cast<const ushort *>(source + 10);	

	uint code = *reinterpret_cast<const uint *>(source + 12);

	uint temp;
	temp = (color0 >> 11) * 255 + 16;
	uchar r0 = (uchar)((temp/32 + temp)/32);
	temp = ((color0 & 0x07E0) >> 5) * 255 + 32;
	uchar g0 = (uchar)((temp/64 + temp)/64);
	temp = (color0 & 0x001F) * 255 + 16;
	uchar b0 = (uchar)((temp/32 + temp)/32);

	temp = (color1 >> 11) * 255 + 16;
	uchar r1 = (uchar)((temp/32 + temp)/32);
	temp = ((color1 & 0x07E0) >> 5) * 255 + 32;
	uchar g1 = (uchar)((temp/64 + temp)/64);
	temp = (color1 & 0x001F) * 255 + 16;
	uchar b1 = (uchar)((temp/32 + temp)/32);

	uint rgb0 = RGB(r0, g0, b0);
	uint rgb1 = RGB(r1, g1, b1);
	uint color2 = RGB((2 * r0 + r1)/3, (2 * g0 + g1)/3, (2 * b0 + b1)/3);
	uint color3 = RGB((r0 + 2 * r1)/3, (g0 + 2 * g1)/3, (b0 + 2 * b1)/3);

	uint colors[4];
	colors[0] = rgb0;
	colors[1] = rgb1;
	colors[2] = color2;
	colors[3] = color3;

	uchar alpha2, alpha3, alpha4, alpha5, alpha6, alpha7;

	if (alpha0 > alpha1)
	{
		alpha2 = (6 * alpha0 + 1 * alpha1)/7;
		alpha3 = (5 * alpha0 + 2 * alpha1)/7;
		alpha4 = (4 * alpha0 + 3 * alpha1)/7;
		alpha5 = (3 * alpha0 + 4 * alpha1)/7;
		alpha6 = (2 * alpha0 + 5 * alpha1)/7;
		alpha7 = (1 * alpha0 + 6 * alpha1)/7;
	}

	else
	{
		alpha2 = (4 * alpha0 + 1 * alpha1)/5;
		alpha3 = (3 * alpha0 + 2 * alpha1)/5;
		alpha4 = (2 * alpha0 + 3 * alpha1)/5;
		alpha5 = (1 * alpha0 + 4 * alpha1)/5;
		alpha6 = 0;
		alpha7 = 255;
	}

	uchar alphas[8];
	alphas[0] = alpha0;
	alphas[1] = alpha1;
	alphas[2] = alpha2;
	alphas[3] = alpha3;
	alphas[4] = alpha4;
	alphas[5] = alpha5;
	alphas[6] = alpha6;
	alphas[7] = alpha7;

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			int alphaCodeIndex = 3*(4*i+j);
			int alphaCode;

			if (alphaCodeIndex <= 12)
			{
				alphaCode = (alphaCode2 >> alphaCodeIndex) & 0x07;
			}
			else if (alphaCodeIndex == 15)
			{
				alphaCode = (alphaCode2 >> 15) | ((alphaCode1 << 1) & 0x06);
			}
			else // alphaCodeIndex >= 18 && alphaCodeIndex <= 45
			{
				alphaCode = (alphaCode1 >> (alphaCodeIndex - 16)) & 0x07;
			}

			uchar alpha = alphas[alphaCode];
			uchar posCode = (code >> 2*(4*i+j)) & 0x03;
			uint color = colors[posCode];
			uint red = RED(color);
			uint green = GREEN(color);
			uint blue = BLUE(color);


			destination[(y + i)*width + (x + j)] = RGBA(red, green, blue, alpha);
		}
	}
	
}

} // namespace Direct3D11
