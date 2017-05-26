#pragma once
//------------------------------------------------------------------------------
/**
	This is the OGL4 implementation of a shader read/write texture.

	This type of texture is only to be used for shaders which do image read/writes, 
	and	not for rendering objects or attaching them to framebuffers.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/shaderreadwritetexturebase.h"
namespace OpenGL4
{
class OGL4ShaderImage : public Base::ShaderReadWriteTextureBase
{
	__DeclareClass(OGL4ShaderImage)
public:
	/// constructor
	OGL4ShaderImage();
	/// destructor
	virtual ~OGL4ShaderImage();

	/// setup texture
	void Setup(const SizeT width, const SizeT height, const CoreGraphics::PixelFormat::Code& format, const Resources::ResourceId& id);

	/// resize texture
	void Resize(SizeT width, SizeT height);
	/// clear texture
	void Clear(const Math::float4& clearColor);
private:
	GLuint ogl4Texture;
};

} // namespace OpenGL4