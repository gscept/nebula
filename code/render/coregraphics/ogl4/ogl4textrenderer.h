#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4TextRenderer
  
    Implements a simple text renderer for OpenGL4. This is only intended
    for outputting debug text, not for high-quality text rendering!
    
    (C) 2012 Radon Labs GmbH
	(C) 2013-2018 Individual contributors, see AUTHORS file
*/    

#include "coregraphics/base/textrendererbase.h"
#include "coregraphics/shaderstate.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/bufferlock.h"

//------------------------------------------------------------------------------
namespace OpenGL4
{
class OGL4TextRenderer : public Base::TextRendererBase
{
    __DeclareClass(OGL4TextRenderer);
    __DeclareSingleton(OGL4TextRenderer);
public:

    struct TextElementVertex
    {
        Math::float2 vertex;
        Math::float2 uv;
        Math::float4 color;
    };

    /// constructor
    OGL4TextRenderer();
    /// destructor
    virtual ~OGL4TextRenderer();

    /// open the device
    void Open();
    /// close the device
    void Close();
    /// draw the accumulated text
    void DrawTextElements();

	static const int MaxNumChars = 4096;

private:

    /// draws text buffer
    void Draw(TextElementVertex* buffer, SizeT numChars);

    /// helper function which moves vertex into proper position
    Math::float2 TransformTextVertex(const Math::float2& pos, const Math::float2& offset, const Math::float2& scale);

	TextElementVertex vertices[MaxNumChars * 6];
	Ptr<CoreGraphics::Shader> shader;
	Ptr<CoreGraphics::ShaderVariable> texVar;
	Ptr<CoreGraphics::ShaderVariable> modelVar;
	Ptr<CoreGraphics::Texture> glyphTexture;
	CoreGraphics::PrimitiveGroup group;
	Ptr<CoreGraphics::VertexBuffer> vbo;
	Ptr<CoreGraphics::BufferLock> bufferLock;
	byte* vertexPtr;

    GLuint glBufferIndex;
};

} // namespace Direct3D9
//------------------------------------------------------------------------------
