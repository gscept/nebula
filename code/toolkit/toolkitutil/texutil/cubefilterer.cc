//------------------------------------------------------------------------------
//  cubefilterer.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "cubefilterer.h"
#include "coregraphics/base/resourcebase.h"
#include "threading/thread.h"
#include "IL/il.h"
#include "IL/ilu.h"
#include "IL/devil_internal_exports.h"
#include "io/ioserver.h"

// these are the defines in DevIL
#define DDS_CUBEMAP_POSITIVEX	0x00000400L
#define DDS_CUBEMAP_NEGATIVEX	0x00000800L
#define DDS_CUBEMAP_POSITIVEY	0x00001000L
#define DDS_CUBEMAP_NEGATIVEY	0x00002000L
#define DDS_CUBEMAP_POSITIVEZ	0x00004000L
#define DDS_CUBEMAP_NEGATIVEZ	0x00008000L

using namespace CoreGraphics;
namespace ToolkitUtil
{

__ImplementClass(ToolkitUtil::CubeFilterer, 'CUFI', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
CubeFilterer::CubeFilterer()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
CubeFilterer::~CubeFilterer()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
CubeFilterer::Filter(bool irradiance, void* messageHandler, void(*CubeFilterer_Progress)(const Util::String&, void*))
{
	n_assert(this->size > 0);
	n_assert(this->cubefaces.Size() == 6);

	SizeT mipLevels;
	if (this->generateMips)
	{
		IndexT i;
		for (i = 0; i < 6; i++) this->cubefaces[i]->GenerateMipmaps();
		mipLevels = (SizeT)(ceil(Math::n_log2(Math::n_max((float)this->cubefaces[0]->GetWidth(), (float)this->cubefaces[0]->GetHeight()))) + 1);
	}
	else mipLevels = 1;	// 1 means we have 1 mip, which is the main texture

	// mip levels should be levels - 1 because it represents the maximum mip index, not the amount of mips
	//this->processor.Init(this->cubeMap->GetWidth(), this->size, mipLevels, 4);

	// eh, init IL every time isn't really THAT nice, but we need to be sure it's running so...
	ilInit();

	// figure out format
	//uint format;
	float gamma = 1.0f;
	uint channels;
	ILenum components;
	ILenum type;
	ILuint byteSize;
	switch (this->cubefaces[0]->GetPixelFormat())
	{
	case PixelFormat::R16G16B16A16:
		//format = CP_VAL_UNORM16;
		channels = 4;
		components = IL_RGBA;
		type = IL_SHORT;
		byteSize = 2;
		break;
	case PixelFormat::R16G16B16A16F:
		//format = CP_VAL_FLOAT16;
		components = IL_RGBA;
		type = IL_HALF;
		channels = 4;
		byteSize = 2;
		break;
	case PixelFormat::R32G32B32A32F:
		//format = CP_VAL_FLOAT32;
		components = IL_RGBA;
		type = IL_FLOAT;
		channels = 4;
		byteSize = 4;
		break;
	case PixelFormat::A8B8G8R8:
		//format = CP_VAL_UNORM8_BGRA;
		components = IL_RGBA;
		type = IL_UNSIGNED_BYTE;
		channels = 4;
		byteSize = 1;
		break;
	case PixelFormat::DXT1sRGB:
		components = IL_RGB;
		channels = 3;
		gamma = 2.2f;
		//format = CP_VAL_UNORM8;
		type = IL_UNSIGNED_BYTE;
		byteSize = 1;
		break;
	case PixelFormat::DXT1AsRGB:		
	case PixelFormat::DXT3sRGB:
	case PixelFormat::DXT5sRGB:
	case PixelFormat::BC7sRGB:
		components = IL_RGBA;
		type = IL_UNSIGNED_BYTE;
		byteSize = 1;
		channels = 4;
		gamma = 2.2f;
	case PixelFormat::DXT1:
	case PixelFormat::DXT1A:
	case PixelFormat::DXT3:
	case PixelFormat::DXT5:
	case PixelFormat::BC7:
	case PixelFormat::R8G8B8A8:
		channels = 4;
		//format = CP_VAL_UNORM8;
		components = IL_RGBA;
		type = IL_UNSIGNED_BYTE;
		byteSize = 1;
		break;
	case PixelFormat::SRGBA8:
		gamma = 2.2f;
	case PixelFormat::R8G8B8:
	case PixelFormat::RGBA8:
		//format = CP_VAL_UNORM8;
		components = IL_RGBA;
		type = IL_UNSIGNED_BYTE;
		byteSize = 1;
		channels = 3;
		break;
	default:
		n_error("Unsupported cube map format!");
		break;
	}

	// load all faces into processor
	IndexT i;
	for (i = 0; i < 6; i++)
	{
		Texture::MapInfo mapInfo;

		// map face, load into processor, then unmap that face
		this->cubefaces[i]->Map(0, Texture::MapRead, mapInfo);
		//this->processor.SetInputFaceData(i, format, channels, mapInfo.rowPitch, mapInfo.data, 16, 1 / gamma, 1);
		this->cubefaces[i]->Unmap(0);
	}

	// perform filtering!
	//this->processor.InitiateFiltering(0, 1, 2, CP_FILTER_TYPE_COSINE, CP_FIXUP_WARP, 4, true, true, this->power, 0.25f, mipLevels, CP_COSINEPOWER_CHAIN_DROP, true, irradiance, CP_LIGHTINGMODEL_PHONG, 10, 1);

	/*
	// wait for thread to be done
	while (this->processor.GetStatus() == CP_STATUS_PROCESSING)
	{
		if (CubeFilterer_Progress != 0 && messageHandler != 0)
		{
			Util::String status = this->processor.GetFilterProgressString();
			CubeFilterer_Progress(status, messageHandler);
		}

		// wait for a certain amount of time to not overload the thread
		n_sleep(0.1f);
	}
	this->processor.RefreshStatus();
	*/

	// time to create the texture
	ILuint image = ilGenImage();
	ilBindImage(image);
	ILimage* internalImage = ilGetCurImage();

	// generate faces
	ilRegisterNumFaces(5);

	// these are the DDS flags in DevIL
	uint faces[] = { DDS_CUBEMAP_POSITIVEX, DDS_CUBEMAP_NEGATIVEX, DDS_CUBEMAP_POSITIVEY, DDS_CUBEMAP_NEGATIVEY, DDS_CUBEMAP_POSITIVEZ, DDS_CUBEMAP_NEGATIVEZ };

	// walk through faces
	for (i = 0; i < 6; i++)
	{
		ilBindImage(image);
		ilActiveFace(i);
		ilSetInteger(IL_IMAGE_CUBEFLAGS, faces[i]);
		
		// if we are using a 16 bit float format, then we have no compression
		ilSetInteger(IL_DXTC_FORMAT, IL_DXGI_UNCOMPRESSED);

		// make room for mips (which is inclusive of the 0th mip)
		ilRegisterNumMips(mipLevels - 1);

		// and through all mips
		IndexT j;
		for (j = 0; j < mipLevels; j++)
		{
			Texture::MapInfo mapInfo;
			this->cubefaces[0]->Map(j, Texture::MapRead, mapInfo);

			// save output
			//this->processor.GetOutputFaceData(i, j, format, channels, mapInfo.rowPitch, mapInfo.data, 1.0f, gamma);

			// load into IL image, then unmap
			ilBindImage(image);
			ilActiveFace(i);
			ilActiveMipmap(j);
			ilTexImageSurface(mapInfo.mipWidth, mapInfo.mipHeight, 1, channels, components, type, mapInfo.data);
			ilSetInteger(IL_IMAGE_ORIGIN, IL_ORIGIN_UPPER_LEFT);

			//ilSetPixels(0, 0, 0, mapInfo.mipWidth, mapInfo.mipHeight, 1, components, type, mapInfo.data);
			this->cubefaces[0]->Unmap(j);
		}
	}
	
	// save!
	IO::IoServer::Instance()->DeleteFile(this->output);
	ilBindImage(image);
	ilSave(IL_DDS, this->output.LocalPath().AsCharPtr());
	ilDeleteImage(image);
}

//------------------------------------------------------------------------------
/**
*/
void
CubeFilterer::DepthCube(bool genConeMap, void* messageHandler, void(*CubeFilterer_Progress)(const Util::String&, void*))
{
	n_assert(this->size > 0);
	n_assert(this->cubefaces.Size() == 6);

	// eh, init IL every time isn't really THAT nice, but we need to be sure it's running so...
	ilInit();

	// figure out format
	float gamma = 1.0f;
	uint channels = 1;
	ILenum components = IL_RED;
	ILenum type = IL_FLOAT;
	ILuint byteSize = sizeof(float);

	// time to create the texture
	ILuint image = ilGenImage();
	ilBindImage(image);
	ILimage* internalImage = ilGetCurImage();

	// generate faces
	ilRegisterNumFaces(5);

	// these are the DDS flags in DevIL
	uint faces[] = { DDS_CUBEMAP_POSITIVEX, DDS_CUBEMAP_NEGATIVEX, DDS_CUBEMAP_POSITIVEY, DDS_CUBEMAP_NEGATIVEY, DDS_CUBEMAP_POSITIVEZ, DDS_CUBEMAP_NEGATIVEZ };

	// if we are using a 16 bit float format, then we have no compression
	ilSetInteger(IL_DXTC_FORMAT, IL_DXGI_UNCOMPRESSED);
	ilRegisterNumMips(0);

	// walk through faces
	IndexT i;
	for (i = 0; i < 6; i++)
	{
		ilBindImage(image);
		ilActiveFace(i);
		ilSetInteger(IL_IMAGE_CUBEFLAGS, faces[i]);
		
		Texture::MapInfo mapInfo;
		this->cubefaces[i]->Map(0, Texture::MapRead, mapInfo);

		// load into IL image, then unmap
		ilActiveMipmap(0);
		ilTexImageSurface(mapInfo.mipWidth, mapInfo.mipHeight, 1, channels, components, type, mapInfo.data);
		ilSetInteger(IL_IMAGE_ORIGIN, IL_ORIGIN_UPPER_LEFT);

		//ilSetPixels(0, 0, 0, mapInfo.mipWidth, mapInfo.mipHeight, 1, components, type, mapInfo.data);
		this->cubefaces[i]->Unmap(0);
	}

	// save!
	IO::IoServer::Instance()->DeleteFile(this->output);
	ilBindImage(image);
	ilSave(IL_DDS, this->output.LocalPath().AsCharPtr());
	ilDeleteImage(image);

	this->cubefaces.Clear();
}

} // namespace ToolkitUtil