//------------------------------------------------------------------------------
// vktextrenderer.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "vktextrenderer.h"
#include "coregraphics/displaydevice.h"
#include "threading/thread.h"
#include "io/ioserver.h"

#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb_truetype.h"
#include "coregraphics/shaderserver.h"
#include "resources/resourceserver.h"

#define FONT_SIZE 32.0f
#define ONEOVERFONTSIZE 1/32.0f
#define GLYPH_TEXTURE_SIZE 1024

using namespace Resources;
using namespace CoreGraphics;
using namespace Threading;
using namespace Math;
namespace Vulkan
{

__ImplementClass(Vulkan::VkTextRenderer, 'VKTR', Base::TextRendererBase);
__ImplementSingleton(Vulkan::VkTextRenderer);
//------------------------------------------------------------------------------
/**
*/
VkTextRenderer::VkTextRenderer()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
VkTextRenderer::~VkTextRenderer()
{
    if (this->IsOpen())
    {
        this->Close();
    }
    __DestructSingleton;
}


//------------------------------------------------------------------------------
/**
*/
void
VkTextRenderer::Open()
{
    n_assert(!this->IsOpen());

    // call parent
    Base::TextRendererBase::Open();

    // setup vbo
    Util::Array<VertexComponent> comps;
    comps.Append(VertexComponent(0, VertexComponent::Float2, 0));
    comps.Append(VertexComponent(1, VertexComponent::Float2, 0));
    comps.Append(VertexComponent(2, VertexComponent::Float4, 0));

    this->layout = CreateVertexLayout({ .name = "Vulakn Text Renderer"_atm, .comps = comps });

    BufferCreateInfo vboInfo;
    vboInfo.name = "TextRenderer VBO"_atm;
    vboInfo.size = MaxNumChars * 6;
    vboInfo.elementSize = VertexLayoutGetSize(this->layout);
    vboInfo.mode = CoreGraphics::HostCached;
    vboInfo.usageFlags = CoreGraphics::VertexBuffer;
    vboInfo.data = nullptr;
    vboInfo.dataSize = 0;
    this->vbo = CreateBuffer(vboInfo);
    this->vertexPtr = (byte*)BufferMap(this->vbo);

    // setup primitive group
    this->group.SetNumIndices(0);
    this->group.SetBaseIndex(0);
    this->group.SetBaseVertex(0);

#if __WIN32__
    // find windows font
    const char* fontPath = "c:/windows/fonts/segoeui.ttf";
#else
    // find linux
    const char* fontPath = "/usr/share/fonts/truetype/freefont/FreeSans.ttf";
#endif

    unsigned char* ttf_buffer = nullptr;

    // load font
    Ptr<IO::Stream> fontStream = IO::IoServer::Instance()->CreateStream(fontPath);
    fontStream->SetAccessMode(IO::Stream::ReadAccess);
    if (fontStream->Open())
    {
        ttf_buffer = new unsigned char[fontStream->GetSize()];
        void* buf = fontStream->Map();
        memcpy(ttf_buffer, buf, fontStream->GetSize());
        fontStream->Unmap();
        fontStream->Close();
    }
    else
    {
        n_error("Failed to load font %s!", fontPath);
    }

    unsigned char* bitmap = new unsigned char[GLYPH_TEXTURE_SIZE*GLYPH_TEXTURE_SIZE];
    int charCount = 96;
    this->cdata = new stbtt_packedchar[charCount];

    stbtt_pack_context context;
    if (!stbtt_PackBegin(&context, bitmap, GLYPH_TEXTURE_SIZE, GLYPH_TEXTURE_SIZE, 0, 1, NULL))
    {
        n_error("VkTextRenderer::Open(): Could not load font!");
    }
    stbtt_PackSetOversampling(&context, 4, 4);
    if (!stbtt_PackFontRange(&context, ttf_buffer, 0, 40.0f, 32, charCount, this->cdata))
    {
        n_error("VkTextRenderer::Open(): Could not load font!");
    }
    stbtt_PackEnd(&context);
    stbtt_InitFont(&this->font, ttf_buffer, 0);

    // setup random texture
    TextureCreateInfo texInfo;
    texInfo.name = "GlyphTexture"_atm;
    texInfo.usage = TextureUsage::SampleTexture;
    texInfo.tag = "render_system"_atm;
    texInfo.data = bitmap;
    texInfo.dataSize = sizeof(unsigned char) * GLYPH_TEXTURE_SIZE * GLYPH_TEXTURE_SIZE;
    texInfo.type = TextureType::Texture2D;
    texInfo.format = PixelFormat::R8;
    texInfo.width = GLYPH_TEXTURE_SIZE;
    texInfo.height = GLYPH_TEXTURE_SIZE;
    this->glyphTexture = CreateTexture(texInfo);

    // create shader instance
    const ShaderId shd = CoreGraphics::ShaderGet("shd:system_shaders/text.gplb");
    this->program = ShaderGetProgram(shd, CoreGraphics::ShaderFeatureMask("Static"));
    this->textTable = ShaderCreateResourceTable(shd, NEBULA_BATCH_GROUP, 1);
    // get variable

    this->texVar = ShaderGetResourceSlot(shd, "Texture");
    ResourceTableSetTexture(this->textTable, { this->glyphTexture, this->texVar, 0, CoreGraphics::InvalidSamplerId, false});
    ResourceTableCommitChanges(this->textTable);
    this->modelVar = ShaderGetConstantBinding(shd, "TextProjectionModel");

    delete[] bitmap;
    delete[] ttf_buffer;
}

//------------------------------------------------------------------------------
/**
*/
void
VkTextRenderer::Close()
{
    n_assert(this->IsOpen());

    // call base class
    Base::TextRendererBase::Close();

    // discard shader
    BufferUnmap(this->vbo);
    this->vertexPtr = nullptr;

    DestroyBuffer(this->vbo);
    DestroyTexture(this->glyphTexture);
}

//------------------------------------------------------------------------------
/**
*/
void
VkTextRenderer::DrawTextElements(const CoreGraphics::CmdBufferId cmdBuf)
{
    n_assert(this->IsOpen());

    // get display mode
    const DisplayMode& displayMode = WindowGetDisplayMode(CoreGraphics::CurrentWindow);

    // calculate projection matrix
#if __VULKAN__
    mat4 proj = orthooffcenterrh(0, (float)displayMode.GetWidth(), (float)displayMode.GetHeight(), 0, -1.0f, +1.0f);
#else
    mat4 proj = orthooffcenterrh(0, (float)displayMode.GetWidth(), 0, (float)displayMode.GetHeight(), -1.0f, +1.0f);
#endif

    // apply shader and apply state
    CoreGraphics::CmdSetShaderProgram(cmdBuf, this->program);
    CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, this->modelVar, sizeof(proj), (byte*)&proj);
    CoreGraphics::CmdSetResourceTable(cmdBuf, this->textTable, NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);

    CoreGraphics::CmdSetVertexLayout(cmdBuf, this->layout);
    CoreGraphics::CmdSetPrimitiveTopology(cmdBuf, CoreGraphics::PrimitiveTopology::TriangleList);
    CoreGraphics::CmdSetGraphicsPipeline(cmdBuf);

    uint screenWidth, screenHeight;
    screenWidth = displayMode.GetWidth();
    screenHeight = displayMode.GetHeight();

    // update vertex buffer
    unsigned vert = 0;

    TextElementVertex vertices[MaxNumChars * 6];

    // draw text elements
    IndexT i;
    for (i = 0; i < this->textElements.Size(); i++)
    {
        const TextElement& curTextElm = this->textElements[i];
        const vec4& color = curTextElm.GetColor();
        const float fontSize = curTextElm.GetSize();
        const unsigned char* text = (unsigned char*)curTextElm.GetText().AsCharPtr();
        vec2 position = curTextElm.GetPosition();

        // calculate ascent, descent and gap for font
        int ascent, descent, gap;
        stbtt_GetFontVMetrics(&this->font, &ascent, &descent, &gap);
        float scale = stbtt_ScaleForPixelHeight(&this->font, fontSize);
        float realSize = Math::ceil((ascent - descent + gap) * scale);

        // transform position
        position = vec2(position.x * screenWidth, position.y * screenHeight + (ascent + descent + gap) * scale + 1);

        float top, left;
        left = 0;
        top = 0;


        // iterate through tokens
        while (*text)
        {
            if (*text >= 32 && *text < 128)
            {
                stbtt_aligned_quad quad;
                stbtt_GetPackedQuad(this->cdata, GLYPH_TEXTURE_SIZE, GLYPH_TEXTURE_SIZE, *text - 32, &left, &top, &quad, 1);

                // create vec4 of data (in order to utilize SSE)
                vec4 sizeScale = vec4(realSize);
                vec4 oneOverFontSize = vec4(ONEOVERFONTSIZE);
                vec4 pos1 = vec4(quad.x0, quad.y0, quad.x1, quad.y0);
                vec4 pos2 = vec4(quad.x1, quad.y1, quad.x0, quad.y0);
                vec4 pos3 = vec4(quad.x1, quad.y1, quad.x0, quad.y1);
                vec4 elementPos = vec4(position.x, position.y, position.x, position.y);

                // basically multiply everything with ONEOVERFONTSIZE and then the realSize, and then append the position
                pos1 = pos1 * oneOverFontSize;
                pos1 = multiplyadd(pos1, sizeScale, elementPos);
                pos2 = pos2 * oneOverFontSize;
                pos2 = multiplyadd(pos2, sizeScale, elementPos);
                pos3 = pos3 * oneOverFontSize;
                pos3 = multiplyadd(pos3, sizeScale, elementPos);

                // vertex 1
                color.storeu(vertices[vert].color.v);
                vertices[vert].uv.x = quad.s0;
                vertices[vert].uv.y = quad.t0;
                vertices[vert].vertex.x = pos1.x;
                vertices[vert].vertex.y = pos1.y;

                // vertex 2
                color.storeu(vertices[vert + 1].color.v);
                vertices[vert + 1].uv.x = quad.s1;
                vertices[vert + 1].uv.y = quad.t0;
                vertices[vert + 1].vertex.x = pos1.z;
                vertices[vert + 1].vertex.y = pos1.w;

                // vertex 3
                color.storeu(vertices[vert + 2].color.v);
                vertices[vert + 2].uv.x = quad.s1;
                vertices[vert + 2].uv.y = quad.t1;
                vertices[vert + 2].vertex.x = pos2.x;
                vertices[vert + 2].vertex.y = pos2.y;

                // vertex 4
                color.storeu(vertices[vert + 3].color.v);
                vertices[vert + 3].uv.x = quad.s0;
                vertices[vert + 3].uv.y = quad.t0;
                vertices[vert + 3].vertex.x = pos2.z;
                vertices[vert + 3].vertex.y = pos2.w;

                // vertex 5
                color.storeu(vertices[vert + 4].color.v);
                vertices[vert + 4].uv.x = quad.s1;
                vertices[vert + 4].uv.y = quad.t1;
                vertices[vert + 4].vertex.x = pos3.x;
                vertices[vert + 4].vertex.y = pos3.y;

                // vertex 6
                color.storeu(vertices[vert + 5].color.v);
                vertices[vert + 5].uv.x = quad.s0;
                vertices[vert + 5].uv.y = quad.t1;
                vertices[vert + 5].vertex.x = pos3.z;
                vertices[vert + 5].vertex.y = pos3.w;

                vert += 6;
            }

            n_assert(vert <= MaxNumChars * 6);
            ++text;
        }
    }

    // if we have vertices left, draw them
    if (vert > 0)
    {
        this->Draw(cmdBuf, vertices, vert / 6);
    }

    // delete the text elements of my own thread id, all other text elements
    // are from other threads and will be deleted through DeleteTextByThreadId()
    this->DeleteTextElementsByThreadId(Thread::GetMyThreadId());
}

//------------------------------------------------------------------------------
/**
*/
void
VkTextRenderer::Draw(const CoreGraphics::CmdBufferId cmdBuf, TextElementVertex* buffer, SizeT numChars)
{
    // copy to buffer
    memcpy(this->vertexPtr, buffer, sizeof(TextElementVertex) * numChars * 6);

    // create primitive group
    this->group.SetNumVertices(numChars * 6);

    // get render device and set it up
    CoreGraphics::CmdSetVertexBuffer(cmdBuf, 0, this->vbo, 0);

    // set viewport
    //CoreGraphics::WindowId wnd = DisplayDevice::Instance()->GetCurrentWindow();
    //const DisplayMode& displayMode = WindowGetDisplayMode(wnd);
    //uint screenWidth, screenHeight;
    //screenWidth = displayMode.GetWidth();
    //screenHeight = displayMode.GetHeight();

    CoreGraphics::BufferFlush(this->vbo);
    //dev->SetViewport(Math::rectangle<int>(0, 0, screenWidth, screenHeight), 0);
    //dev->SetScissorRect(Math::rectangle<int>(0, 0, screenWidth, screenHeight), 0);

    // setup device
    CoreGraphics::CmdDraw(cmdBuf, this->group);
}

//------------------------------------------------------------------------------
/**
*/
Math::vec2
VkTextRenderer::TransformTextVertex(const Math::vec2& pos, const Math::vec2& offset, const Math::vec2& scale)
{
    return vec2::multiply(vec2::multiply(pos, offset), scale);
}

} // namespace Vulkan
