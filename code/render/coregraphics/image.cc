//------------------------------------------------------------------------------
//  image.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/config.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "image.h"
#include "IL/il.h"
#include "il_dds.h"
namespace CoreGraphics
{

ImageAllocator imageAllocator;
//------------------------------------------------------------------------------
/**
*/
ImageId CreateImage(const ImageCreateInfoFile& info)
{
	// open file, yes, synced
	Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(info.path);
	stream->SetAccessMode(IO::Stream::ReadAccess);
	if (stream->Open())
	{
		void* srcData = stream->Map();
		uint srcDataSize = stream->GetSize();

		Ids::Id32 id = imageAllocator.Alloc();
		ImageLoadInfo& loadInfo = imageAllocator.Get<0>(id);

		// create IL image
		ILuint image = ilGenImage();
		ilBindImage(image);

		Util::String ext = info.path.AsString().GetFileExtension();

		// use extension to figure out what image we should open
		if (ext == "dds")
		{
			// loading a dds will automatically decompress it
			ilSetInteger(IL_DXTC_NO_DECOMPRESS, IL_FALSE);
			ilLoadL(IL_DDS, srcData, srcDataSize);
			loadInfo.container = DDS;
		}
		else if (ext == "png")
		{
			ilLoadL(IL_PNG, srcData, srcDataSize);
			loadInfo.container = PNG;
		}
		else if (ext == "jpg" || ext == "jpeg")
		{
			ilLoadL(IL_JPG, srcData, srcDataSize);
			loadInfo.container = JPEG;
		}

		if (info.convertTo32Bit)
			ilConvertImage(ilGetInteger(IL_PIXEL_FORMAT), IL_UNSIGNED_INT);

		ILuint width = ilGetInteger(IL_IMAGE_WIDTH);
		ILuint height = ilGetInteger(IL_IMAGE_HEIGHT);
		ILuint depth = ilGetInteger(IL_IMAGE_DEPTH);
		ILuint bpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
		ILuint channels = ilGetInteger(IL_IMAGE_CHANNELS);
		ILenum format = ilGetInteger(IL_PIXEL_FORMAT);

		PixelFormat::Code fmt = PixelFormat::InvalidPixelFormat;
		switch (format)
		{
		case PF_ARGB:				
		{ 
			fmt = PixelFormat::R8G8B8A8; 
			loadInfo.redOffset = 0;
			loadInfo.greenOffset = 1;
			loadInfo.blueOffset = 2;
			loadInfo.alphaOffset = 3;
			loadInfo.primitive = Bit8Uint;
			break;
		}
		case PF_RGB:				
		{ 
			fmt = PixelFormat::R8G8B8; 
			loadInfo.redOffset = 0;
			loadInfo.greenOffset = 1;
			loadInfo.blueOffset = 2;
			loadInfo.alphaOffset = -1;
			loadInfo.primitive = Bit8Uint;
			break; 
		}
		case PF_DXT1:				
		{ 
			fmt = PixelFormat::DXT1; 
			loadInfo.redOffset = 0;
			loadInfo.greenOffset = 1;
			loadInfo.blueOffset = 2;
			loadInfo.alphaOffset = -1;
			loadInfo.primitive = Bit8Uint;
			break; 
		}
		case PF_DXT3:				
		{ 
			fmt = PixelFormat::DXT3; 
			loadInfo.redOffset = 0;
			loadInfo.greenOffset = 1;
			loadInfo.blueOffset = 2;
			loadInfo.alphaOffset = 3;
			loadInfo.primitive = Bit8Uint;
			break; 
		}
		case PF_DXT5:				
		{ 
			fmt = PixelFormat::DXT5; 
			loadInfo.redOffset = 0;
			loadInfo.greenOffset = 1;
			loadInfo.blueOffset = 2;
			loadInfo.alphaOffset = 3;
			loadInfo.primitive = Bit8Uint;
			break; 
		}
		// case PF_BC7:				{ fmt = PixelFormat::BC7; break; } not supported
		case PF_DXT1_sRGB:			
		{ 
			fmt = PixelFormat::DXT1sRGB; 
			loadInfo.redOffset = 0;
			loadInfo.greenOffset = 1;
			loadInfo.blueOffset = 2;
			loadInfo.alphaOffset = -1;
			loadInfo.primitive = Bit8Uint;
			break; 
		}
		case PF_DXT3_sRGB:			
		{ 
			fmt = PixelFormat::DXT3sRGB; 
			loadInfo.redOffset = 0;
			loadInfo.greenOffset = 1;
			loadInfo.blueOffset = 2;
			loadInfo.alphaOffset = 3;
			loadInfo.primitive = Bit8Uint;
			break; 
		}
		case PF_DXT5_sRGB:			
		{ 
			fmt = PixelFormat::DXT5sRGB; 
			loadInfo.redOffset = 0;
			loadInfo.greenOffset = 1;
			loadInfo.blueOffset = 2;
			loadInfo.alphaOffset = 3;
			loadInfo.primitive = Bit8Uint;
			break; 
		}
		// case PF_BC7_sRGB:			{ fmt = PixelFormat::BC7sRGB; break; } not supported
		// case PF_3DC:			    { fmt = PixelFormat::BC; break; } // not supported
		case PF_A16R16G16B16:		
		{ 
			fmt = PixelFormat::R16G16B16A16; 
			loadInfo.alphaOffset = 0; 
			loadInfo.redOffset = 2; 
			loadInfo.greenOffset = 4;
			loadInfo.blueOffset = 6;
			loadInfo.primitive = Bit16Uint;
			break; 
		}
		case PF_A16B16G16R16:		
		{ 
			fmt = PixelFormat::R16G16B16A16; 
			loadInfo.alphaOffset = 0;
			loadInfo.blueOffset = 2;
			loadInfo.greenOffset = 4;
			loadInfo.redOffset = 6;
			loadInfo.primitive = Bit16Uint;
			break; 
		}
		case PF_A16R16B16G16F:		
		{ 
			fmt = PixelFormat::R16G16B16A16F; 
			loadInfo.alphaOffset = 0;
			loadInfo.redOffset = 2;
			loadInfo.greenOffset = 4;
			loadInfo.blueOffset = 6;
			loadInfo.primitive = Bit16Float;
			break; 
		}
		case PF_A16B16G16R16F:		
		{ 
			fmt = PixelFormat::R16G16B16A16F; 
			loadInfo.alphaOffset = 0;
			loadInfo.blueOffset = 2;
			loadInfo.greenOffset = 4;
			loadInfo.redOffset = 6;
			loadInfo.primitive = Bit16Float;
			break; 
		}
		case PF_A32B32G32R32F:		
		{ 
			fmt = PixelFormat::R32G32B32A32F; 
			loadInfo.alphaOffset = 0;
			loadInfo.blueOffset = 4;
			loadInfo.greenOffset = 8;
			loadInfo.redOffset = 12;
			loadInfo.primitive = Bit32Float;
			break; 
		}
		case PF_A32R32G32B32F:		
		{ 
			fmt = PixelFormat::R32G32B32A32F; 
			loadInfo.alphaOffset = 0;
			loadInfo.redOffset = 4;
			loadInfo.greenOffset = 8;
			loadInfo.blueOffset = 12;
			loadInfo.primitive = Bit32Float;
			break; 
		}
		case PF_R16F:				
		{ 
			fmt = PixelFormat::R16F; 
			loadInfo.redOffset = 0;
			loadInfo.greenOffset = -1;
			loadInfo.blueOffset = -1;
			loadInfo.alphaOffset = -1;
			loadInfo.primitive = Bit16Float;
			break;
		}
		case PF_R32F:				
		{ 
			fmt = PixelFormat::R32F; 
			loadInfo.redOffset = 0;
			loadInfo.greenOffset = -1;
			loadInfo.blueOffset = -1;
			loadInfo.alphaOffset = -1;
			loadInfo.primitive = Bit32Float;
			break; 
		}
		case PF_G16R16F:			
		{ 
			fmt = PixelFormat::R16G16F; 
			loadInfo.redOffset = 0;
			loadInfo.greenOffset = 2;
			loadInfo.blueOffset = -1;
			loadInfo.alphaOffset = -1;
			loadInfo.primitive = Bit16Float;
			break; 
		}
		case PF_G32R32F:			
		{ 
			fmt = PixelFormat::R32G32F; 
			loadInfo.redOffset = 0;
			loadInfo.greenOffset = 4;
			loadInfo.blueOffset = -1;
			loadInfo.alphaOffset = -1;
			loadInfo.primitive = Bit32Float;
			break; 
		}
		case PF_UNKNOWN:			{ fmt = PixelFormat::InvalidPixelFormat; break; }
		}

		// if the pixelformat is invalid, figure out based on channels and bpp
		if (fmt == PixelFormat::InvalidPixelFormat)
		{
			ILuint bpc = bpp / channels;
			loadInfo.redOffset = 0;
			loadInfo.greenOffset = bpc;
			loadInfo.blueOffset = bpc*2;
			loadInfo.alphaOffset = bpc*3;
			switch (channels)
			{
			case 1:
				switch (bpp)
				{
				case 1:
					fmt = PixelFormat::R8;
					loadInfo.primitive = Bit8Uint;
					break;
				case 2:
					fmt = PixelFormat::R16;
					loadInfo.primitive = Bit16Uint;
					break;
				case 4:
					fmt = PixelFormat::R32;
					loadInfo.primitive = Bit32Uint;
					break;
				}
				break;
			case 2:
				switch (bpp)
				{
				case 1:
					fmt = PixelFormat::InvalidPixelFormat;
					break;
				case 2:
					fmt = PixelFormat::R16G16;
					loadInfo.primitive = Bit16Uint;
					break;
				case 4:
					fmt = PixelFormat::R32G32;
					loadInfo.primitive = Bit32Uint;
					break;
				}
				break;
			case 3:
				switch (bpp)
				{
				case 1:
					fmt = PixelFormat::R8G8B8;
					loadInfo.primitive = Bit8Uint;
					break;
				case 2:
					fmt = PixelFormat::InvalidPixelFormat;
					break;
				case 4:
					fmt = PixelFormat::InvalidPixelFormat;
					break;
				}
				break;
			case 4:
				switch (bpp)
				{
				case 1:
					fmt = PixelFormat::R8G8B8A8;
					loadInfo.primitive = Bit8Uint;
					break;
				case 2:
					fmt = PixelFormat::R16G16B16A16;
					loadInfo.primitive = Bit16Uint;
					break;
				case 4:
					fmt = PixelFormat::R32G32B32A32;
					loadInfo.primitive = Bit32Uint;
					break;
				}
				break;
			}
		}

		n_assert2(fmt != PixelFormat::InvalidPixelFormat, "Pixel format could not be deduced");

		loadInfo.width = width;
		loadInfo.height = height;
		loadInfo.depth = depth;
		loadInfo.format = fmt;

		// make copy of buffer
		ILuint size = ilGetInteger(IL_IMAGE_SIZE_OF_DATA);
		ILubyte* buf = ilGetData();
		loadInfo.buffer = (byte*)Memory::Alloc(Memory::ResourceHeap, size);
		memcpy(loadInfo.buffer, buf, size);

		ImageId ret;
		ret.id24 = id;
		ret.id8 = ImageIdType;
		return ret;
	}
	return ImageId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
ImageId CreateImage(const ImageCreateInfoData& info)
{
	return ImageId();
}

//------------------------------------------------------------------------------
/**
*/
void DestroyImage(const ImageId id)
{
	ImageLoadInfo& loadInfo = imageAllocator.Get<0>(id.id24);
	Memory::Free(Memory::ResourceHeap, loadInfo.buffer);
}

//------------------------------------------------------------------------------
/**
*/
ImageDimensions ImageGetDimensions(const ImageId id)
{
	ImageLoadInfo& loadInfo = imageAllocator.Get<0>(id.id24);
	return ImageDimensions{ loadInfo.width, loadInfo.height, loadInfo.depth };
}

//------------------------------------------------------------------------------
/**
*/
const byte* 
ImageGetBuffer(const ImageId id)
{
	ImageLoadInfo& loadInfo = imageAllocator.Get<0>(id.id24);
	return loadInfo.buffer;
}

//------------------------------------------------------------------------------
/**
*/
const byte* 
ImageGetRedPtr(const ImageId id)
{
	ImageLoadInfo& loadInfo = imageAllocator.Get<0>(id.id24);
	return &loadInfo.buffer[loadInfo.redOffset];
}

//------------------------------------------------------------------------------
/**
*/
const byte* 
ImageGetGreenPtr(const ImageId id)
{
	ImageLoadInfo& loadInfo = imageAllocator.Get<0>(id.id24);
	return &loadInfo.buffer[loadInfo.greenOffset];
}

//------------------------------------------------------------------------------
/**
*/
const byte* 
ImageGetBluePtr(const ImageId id)
{
	ImageLoadInfo& loadInfo = imageAllocator.Get<0>(id.id24);
	return &loadInfo.buffer[loadInfo.blueOffset];
}

//------------------------------------------------------------------------------
/**
*/
const byte* 
ImageGetAlphaPtr(const ImageId id)
{
	ImageLoadInfo& loadInfo = imageAllocator.Get<0>(id.id24);
	return &loadInfo.buffer[loadInfo.alphaOffset];
}

//------------------------------------------------------------------------------
/**
*/
const SizeT 
ImageGetPixelStride(const ImageId id)
{
	ImageLoadInfo& loadInfo = imageAllocator.Get<0>(id.id24);
	return CoreGraphics::PixelFormat::ToSize(loadInfo.format);
}

//------------------------------------------------------------------------------
/**
*/
ImageChannelPrimitive
ImageGetChannelPrimitive(const ImageId id)
{
	ImageLoadInfo& loadInfo = imageAllocator.Get<0>(id.id24);
	return loadInfo.primitive;
}


} // namespace CoreGraphics
