#pragma once
//------------------------------------------------------------------------------
/**
    Implements a text renderer using Vulkan.
    
    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/textrendererbase.h"
#include "coregraphics/shader.h"
#include "coregraphics/texture.h"
#include "coregraphics/primitivegroup.h"
#include "coregraphics/resourcetable.h"
#include "stb_truetype.h"
namespace Vulkan
{
class VkTextRenderer : public Base::TextRendererBase
{
    __DeclareClass(VkTextRenderer);
    __DeclareSingleton(VkTextRenderer);
public:

    struct TextElementVertex
    {
        Math::float2 vertex;
        Math::float2 uv;
        Math::float4 color;
    };

    /// constructor
    VkTextRenderer();
    /// destructor
    virtual ~VkTextRenderer();

    /// open the device
    void Open();
    /// close the device
    void Close();
    /// draw the accumulated text
    void DrawTextElements(const CoreGraphics::CmdBufferId cmdBuf);

    static const int MaxNumChars = 65535;
private:


    /// draws text buffer
    void Draw(const CoreGraphics::CmdBufferId cmdBuf, TextElementVertex* buffer, SizeT numChars);

    /// helper function which moves vertex into proper position
    Math::vec2 TransformTextVertex(const Math::vec2& pos, const Math::vec2& offset, const Math::vec2& scale);

    // define tff buffer
    stbtt_packedchar* cdata;
    stbtt_fontinfo font;

    CoreGraphics::ResourceTableId textTable;
    CoreGraphics::ShaderProgramId program;
    IndexT texVar;
    IndexT modelVar;
    CoreGraphics::TextureId glyphTexture;
    CoreGraphics::PrimitiveGroup group;
    CoreGraphics::BufferId vbo;
    CoreGraphics::VertexLayoutId layout;
    byte* vertexPtr;
};
} // namespace Vulkan
