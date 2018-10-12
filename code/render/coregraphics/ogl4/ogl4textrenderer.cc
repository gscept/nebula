//------------------------------------------------------------------------------
//  OGL4TextRenderer.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "stdneb.h"
#include "coregraphics/ogl4/ogl4textrenderer.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/displaydevice.h"
#include "threading/thread.h"

#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb/stb_truetype.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/memorytextureloader.h"
#include "resources/resourcemanager.h"
#include "coregraphics/memoryvertexbufferloader.h"

namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4TextRenderer, 'D1TR', Base::TextRendererBase);
__ImplementSingleton(OpenGL4::OGL4TextRenderer);

using namespace Resources;
using namespace CoreGraphics;
using namespace Threading;
using namespace Math;


#define FONT_SIZE 32.0f
#define ONEOVERFONTSIZE 1/32.0f
#define GLYPH_TEXTURE_SIZE 768
//------------------------------------------------------------------------------
/**
*/
OGL4TextRenderer::OGL4TextRenderer()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
OGL4TextRenderer::~OGL4TextRenderer()
{
    if (this->IsOpen())
    {
        this->Close();
    }
    __DestructSingleton;
}

// define tff buffer
unsigned char* ttf_buffer;
stbtt_bakedchar cdata[96];
stbtt_fontinfo font;

//------------------------------------------------------------------------------
/**
*/
void
OGL4TextRenderer::Open()
{
    n_assert(!this->IsOpen());

    // call parent
    Base::TextRendererBase::Open();

	// setup vbo
	Util::Array<VertexComponent> comps;
	comps.Append(VertexComponent(VertexComponent::Position, 0, VertexComponent::Float2, 0));
	comps.Append(VertexComponent(VertexComponent::TexCoord1, 0, VertexComponent::Float2, 0));
	comps.Append(VertexComponent(VertexComponent::Color, 0, VertexComponent::Float4, 0));
	Ptr<MemoryVertexBufferLoader> vboLoader = MemoryVertexBufferLoader::Create();
    vboLoader->Setup(comps, MaxNumChars * 6 * 3, NULL, 0, VertexBuffer::UsageDynamic, VertexBuffer::AccessWrite, VertexBuffer::SyncingCoherentPersistent);

	// create vbo and load
	this->vbo = VertexBuffer::Create();
	this->vbo->SetLoader(vboLoader.upcast<ResourceLoader>());
	this->vbo->SetAsyncEnabled(false);
	this->vbo->Load();
	n_assert(this->vbo->IsLoaded());
	this->vbo->SetLoader(NULL);

	// create buffer lock
	this->bufferLock = BufferLock::Create();

	// map buffer
	this->vertexPtr = (byte*)this->vbo->Map(VertexBuffer::MapWrite);
	this->glBufferIndex = 0;

	// setup primitive group
	this->group.SetPrimitiveTopology(PrimitiveTopology::TriangleList);

	// read font buffer
	ttf_buffer = n_new_array(unsigned char, 1<<25);
#if __WIN32__
	// read font from windows
	fread(ttf_buffer, 1, 1<<25, fopen("c:/windows/fonts/segoeui.ttf", "rb"));
#else
    fread(ttf_buffer, 1, 1<<25, fopen("/usr/share/fonts/truetype/freefont/FreeSans.ttf", "rb"));
#endif
	unsigned char bitmap[GLYPH_TEXTURE_SIZE*GLYPH_TEXTURE_SIZE];
	stbtt_InitFont(&font, ttf_buffer, 0);
	stbtt_BakeFontBitmap(ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0), 48.0f, bitmap, GLYPH_TEXTURE_SIZE, GLYPH_TEXTURE_SIZE, 32, 96, cdata);

	// setup random texture
	this->glyphTexture = ResourceManager::Instance()->CreateUnmanagedResource("GlyphTexture", Texture::RTTI).downcast<Texture>();
	Ptr<MemoryTextureLoader> loader = MemoryTextureLoader::Create();
	loader->SetImageBuffer(bitmap, GLYPH_TEXTURE_SIZE, GLYPH_TEXTURE_SIZE, PixelFormat::R8);
	this->glyphTexture->SetLoader(loader.upcast<ResourceLoader>());
	this->glyphTexture->SetResourceId("GlyphTexture");
	this->glyphTexture->SetAsyncEnabled(false);
	this->glyphTexture->Load();
	n_assert(this->glyphTexture->IsLoaded());
	this->glyphTexture->SetLoader(0);

	// create shader instance
	this->shader = ShaderServer::Instance()->GetShader("shd:text");
	this->shader->SelectActiveVariation(ShaderServer::Instance()->FeatureStringToMask("Static"));

	// get variable
	this->texVar = this->shader->GetVariableByName("Texture");
	this->modelVar = this->shader->GetVariableByName("TextProjectionModel");
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4TextRenderer::Close()
{
    n_assert(this->IsOpen());

    // call base class
    Base::TextRendererBase::Close();

	// discard shader
	this->shader = 0;

	// unmap buffer
	this->vbo->Unmap();
	this->vertexPtr = 0;

	// discard vbo
	this->vbo->Unload();
	this->vbo = 0;
	this->bufferLock = 0;

	// unload texture
	this->glyphTexture->Unload();
	this->glyphTexture = 0;

	n_delete_array(ttf_buffer);
}

//------------------------------------------------------------------------------
/**
    Draw buffered text. This method is called once per frame.
*/
void
OGL4TextRenderer::DrawTextElements()
{
	n_assert(this->IsOpen());
	
	// get display mode
	const DisplayMode& displayMode = DisplayDevice::Instance()->GetCurrentWindow()->GetDisplayMode();

	// get render device and set it up
	Ptr<RenderDevice> dev = RenderDevice::Instance();
	dev->SetStreamVertexBuffer(0, this->vbo, 0);
	dev->SetVertexLayout(this->vbo->GetVertexLayout());

    // calculate projection matrix
    matrix44 proj = matrix44::orthooffcenterrh(0, (float)displayMode.GetWidth(), (float)displayMode.GetHeight(), 0, -1.0f, +1.0f);

	// apply shader
    this->shader->Apply();

    // update shader state
    this->shader->BeginUpdate();
	this->texVar->SetTexture(this->glyphTexture);
    this->modelVar->SetMatrix(proj);
    this->shader->EndUpdate();

    // commit changes
    this->shader->Commit();

    // set viewport
    uint screenWidth, screenHeight;
    screenWidth = displayMode.GetWidth();
    screenHeight = displayMode.GetHeight();
    glViewport(0, 0, screenWidth, screenHeight);

    // create vertex buffer
    unsigned vert = 0;
    unsigned totalChars = 0;

	// draw text elements
	IndexT i;
	for (i = 0; i < this->textElements.Size(); i++)
	{
		const TextElement& curTextElm = this->textElements[i];
		const float4& color = curTextElm.GetColor();
		const float fontSize = curTextElm.GetSize();
		const unsigned char* text = (unsigned char*)curTextElm.GetText().AsCharPtr();
		const unsigned numChars = curTextElm.GetText().Length();
        totalChars += numChars;
		float2 position = curTextElm.GetPosition();

        // calculate ascent, descent and gap for font
        int ascent, descent, gap;
        stbtt_GetFontVMetrics(&font, &ascent, &descent, &gap);
        float scale = stbtt_ScaleForPixelHeight(&font, fontSize);
        float realSize = ceil((ascent - descent + gap) * scale);

        // transform position
        position = float2(position.x() * screenWidth, position.y() * screenHeight + (ascent + descent + gap) * scale + 1);

		float top, left; 
		left = 0;
		top = 0;

        // iterate through tokens
		while (*text)
		{
			if (*text >= 32 && *text < 128)
			{
				stbtt_aligned_quad quad;
				stbtt_GetBakedQuad(cdata, GLYPH_TEXTURE_SIZE, GLYPH_TEXTURE_SIZE, *text-32, &left, &top, &quad, 1);

				float width, height;
				width = quad.x1 - quad.x0;
				height = quad.y1 - quad.y0;

                // create float4 of data (in order to utilize SSE)
                float4 sizeScale = float4(realSize);
                float4 oneOverFontSize = float4(ONEOVERFONTSIZE);
                float4 pos1 = float4(quad.x0, quad.y0, quad.x1, quad.y0);
                float4 pos2 = float4(quad.x1, quad.y1, quad.x0, quad.y0);
                float4 pos3 = float4(quad.x1, quad.y1, quad.x0, quad.y1);
                float4 elementPos = float4(position.x(), position.y(), position.x(), position.y());

                // basically multiply everything with ONEOVERFONTSIZE and then the realSize, and then append the position
                pos1 = float4::multiply(pos1, oneOverFontSize);
                pos1 = float4::multiplyadd(pos1, sizeScale, elementPos);
                pos2 = float4::multiply(pos2, oneOverFontSize);
                pos2 = float4::multiplyadd(pos2, sizeScale, elementPos);
                pos3 = float4::multiply(pos3, oneOverFontSize);
                pos3 = float4::multiplyadd(pos3, sizeScale, elementPos);

                // vertex 1
				this->vertices[vert].color = color;
				this->vertices[vert].uv.x() = quad.s0;
				this->vertices[vert].uv.y() = quad.t0;
				this->vertices[vert].vertex.x() = pos1.x();
				this->vertices[vert].vertex.y() = pos1.y();             

                // vertex 2
				this->vertices[vert+1].color = color;
				this->vertices[vert+1].uv.x() = quad.s1;
				this->vertices[vert+1].uv.y() = quad.t0;
				this->vertices[vert+1].vertex.x() = pos1.z();
				this->vertices[vert+1].vertex.y() = pos1.w();             

                // vertex 3
				this->vertices[vert+2].color = color;
				this->vertices[vert+2].uv.x() = quad.s1;
				this->vertices[vert+2].uv.y() = quad.t1;
				this->vertices[vert+2].vertex.x() = pos2.x();
				this->vertices[vert+2].vertex.y() = pos2.y();             

                // vertex 4
				this->vertices[vert+3].color = color;
				this->vertices[vert+3].uv.x() = quad.s0;
				this->vertices[vert+3].uv.y() = quad.t0;
				this->vertices[vert+3].vertex.x() = pos2.z();
				this->vertices[vert+3].vertex.y() = pos2.w();              

                // vertex 5
				this->vertices[vert+4].color = color;
				this->vertices[vert+4].uv.x() = quad.s1;
				this->vertices[vert+4].uv.y() = quad.t1;
				this->vertices[vert+4].vertex.x() = pos3.x();
				this->vertices[vert+4].vertex.y() = pos3.y();              

                // vertex 6
				this->vertices[vert+5].color = color;
				this->vertices[vert+5].uv.x() = quad.s0;
				this->vertices[vert+5].uv.y() = quad.t1;
				this->vertices[vert+5].vertex.x() = pos3.z();
				this->vertices[vert+5].vertex.y() = pos3.w();             

				vert += 6;
			}

            if (vert == MaxNumChars * 6)
            {
				this->Draw(this->vertices, MaxNumChars);
                vert = 0;
            }
			++text;
		}
	}	

    // if we have vertices left, draw them
    if (vert > 0)
    {
		this->Draw(this->vertices, vert / 6);
    }

	// delete the text elements of my own thread id, all other text elements
	// are from other threads and will be deleted through DeleteTextByThreadId()
	this->DeleteTextElementsByThreadId(Thread::GetMyThreadId());
}

//------------------------------------------------------------------------------
/**
*/
Math::float2 
OGL4TextRenderer::TransformTextVertex( const float2& pos, const float2& offset, const float2& scale )
{
    return float2::multiply(float2::multiply(pos, offset), scale);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4TextRenderer::Draw(TextElementVertex* buffer, SizeT numChars)
{
	// get render device
	Ptr<RenderDevice> dev = RenderDevice::Instance();	

	// wait for range to be available
	this->bufferLock->WaitForRange(this->glBufferIndex * sizeof(TextElementVertex) * MaxNumChars * 6, sizeof(TextElementVertex) * numChars * 6);

	// copy to buffer
	memcpy(this->vertexPtr + this->glBufferIndex * sizeof(TextElementVertex) * MaxNumChars * 6, buffer, sizeof(TextElementVertex) * numChars * 6);

	// create primitive group
	this->group.SetBaseVertex(MaxNumChars * 6 * this->glBufferIndex);
	this->group.SetNumVertices(numChars * 6);

	// setup device
	dev->SetPrimitiveGroup(this->group);
	dev->Draw();

	// add new lock
	this->bufferLock->LockRange(this->glBufferIndex * sizeof(TextElementVertex) * MaxNumChars * 6, sizeof(TextElementVertex) * numChars * 6);
    this->glBufferIndex = (this->glBufferIndex + 1) % 3; // we are using triple buffering so we must mod by 3
}
} // namespace OpenGL4

