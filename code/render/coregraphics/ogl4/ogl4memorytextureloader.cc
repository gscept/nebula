//------------------------------------------------------------------------------
//  ogl4memorytextureloader.cc
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/pixelformat.h"
#include "coregraphics/streamtextureloader.h"
#include "coregraphics/texture.h"
#include "coregraphics/renderdevice.h"
#include "ogl4memorytextureloader.h"
#include "coregraphics/ogl4/ogl4types.h"

using namespace CoreGraphics;
using namespace Resources;

namespace OpenGL4
{
__ImplementClass(OGL4MemoryTextureLoader, 'O4MT', Resources::ResourceLoader);

//------------------------------------------------------------------------------------
/**
*/
void
OGL4MemoryTextureLoader::SetImageBuffer(const void* buffer, SizeT width, SizeT height, CoreGraphics::PixelFormat::Code format)
{
	// create texture
	glGenTextures(1, &this->texture);
	glBindTexture(GL_TEXTURE_2D, this->texture);
	glTexImage2D(
				GL_TEXTURE_2D,								// target
				0,											// miplevels
				OGL4Types::AsOGL4PixelFormat(format),		// internal format
				width,										// width
				height,										// height
				0,											// border
				OGL4Types::AsOGL4PixelComponents(format),	// pixel components
				OGL4Types::AsOGL4PixelType(format),			// pixel type
				buffer										// data pointer
				);
	glBindTexture(GL_TEXTURE_2D, 0);

	this->format = format;
	if (!GLSUCCESS)
	{
		n_error("OGL4MemoryTextureLoader: CreateTexture2D() failed");
		return;
	}
}

//------------------------------------------------------------------------------------
/**
*/
bool
OGL4MemoryTextureLoader::OnLoadRequested()
{
	n_assert(this->resource->IsA(Texture::RTTI));
	n_assert(this->texture != 0);
	const Ptr<Texture>& res = this->resource.downcast<Texture>();
	n_assert(!res->IsLoaded());
	res->SetupFromOGL4Texture(this->texture, this->format);
	res->SetState(Resource::Loaded);
	this->SetState(Resource::Loaded);
	return true;		
}
}
