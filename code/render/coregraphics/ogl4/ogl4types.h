#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4Types
    
    Provides static helper functions to convert from and to Direct3D
    data types and enumerations.
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "coregraphics/pixelformat.h"
#include "coregraphics/vertexcomponent.h"
#include "coregraphics/primitivetopology.h"
#include "coregraphics/antialiasquality.h"
#include "coregraphics/imagefileformat.h"
#include "coregraphics/indextype.h"
#include "coregraphics/base/resourcebase.h"
#include <IL/il.h>

//------------------------------------------------------------------------------
namespace OpenGL4
{
class OGL4Types
{
public:
    /// convert Nebula pixel format to OGL4 pixel format
    static GLenum AsOGL4PixelFormat(CoreGraphics::PixelFormat::Code p);
	/// convert Nebula pixel format to OGL4 image layout
	static GLenum AsOGL4PixelComponents(CoreGraphics::PixelFormat::Code p);
	/// convert Nebula pixel format to OGL4 pixel type
	static GLenum AsOGL4PixelType(CoreGraphics::PixelFormat::Code p);
	/// convert Nebula pixel format to OpenGL pixel pack alignment
	static GLuint AsOGL4PixelByteAlignment(CoreGraphics::PixelFormat::Code p);
    /// convert OpenGL4 to Nebula pixel format
    static CoreGraphics::PixelFormat::Code AsNebulaPixelFormat(GLenum f);
	/// convert IL compression to gl compression
	static GLenum AsOGL4Compression(ILenum f);
	/// convert vertex component type to OGL4 symbolic type (single-element)
    static GLenum AsOGL4SymbolicType(CoreGraphics::VertexComponent::Format f);
	/// convert vertex format to size
	static GLint AsOGL4Size(CoreGraphics::VertexComponent::Format f);
	/// convert vertex format to number of components
	static GLint AsOGL4NumComponents(CoreGraphics::VertexComponent::Format f);
    /// convert vertex component semantic name as OGL4 declaration usage
    static GLenum AsOGL4VertexDeclarationUsage(CoreGraphics::VertexComponent::SemanticName n);
	/// convert the format to it's size
	static GLuint AsByteSize(GLuint semantic);
    /// convert primitive topology to D3D
    static GLenum AsOGL4PrimitiveType(CoreGraphics::PrimitiveTopology::Code t);
    /// convert antialias quality to D3D multisample type
    static GLuint AsOGL4MultiSampleType(CoreGraphics::AntiAliasQuality::Code c);
	/// convert Nebula access to OGL4 access
	static GLuint AsOGL4Access( Base::ResourceBase::Access access);
	/// convert Nebula usage to OGL4 usage
	static GLuint AsOGL4Usage(Base::ResourceBase::Usage usage, Base::ResourceBase::Access access);
	/// convert Nebula syncing to OGL4 syncing
	static GLuint AsOGL4Syncing(Base::ResourceBase::Syncing syncing);
    /// convert index type to DXGI format
    static GLenum IndexTypeAsOGL4Format(CoreGraphics::IndexType::Code indexType);
};

} // namespace OpenGL4
//------------------------------------------------------------------------------
