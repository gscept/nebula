#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4MemoryTextureLoader
    
    OpenGL4 texture loader which takes a memory buffer as input.
    
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourceloader.h"
#include "coregraphics/pixelformat.h"

namespace OpenGL4
{
class OGL4MemoryTextureLoader : public Resources::ResourceLoader
{
	__DeclareClass(OGL4MemoryTextureLoader);
public:		
	/// sets the image buffer
	void SetImageBuffer(const void* buffer, SizeT width, SizeT height, CoreGraphics::PixelFormat::Code format);		

	/// on load callback
	virtual bool OnLoadRequested();
private:
	CoreGraphics::PixelFormat::Code format;
	SizeT width, height;
	GLuint texture;
};
} // namespace OpenGL4