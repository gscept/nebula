//------------------------------------------------------------------------------
// vktextrenderer.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vktextrenderer.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/displaydevice.h"
#include "threading/thread.h"

#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb/stb_truetype.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/memorytextureloader.h"
#include "resources/resourcemanager.h"
#include "coregraphics/memoryvertexbufferloader.h"

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
VkTextRenderer::VkTextRenderer() :
	shader(NULL),
	texVar(NULL),
	modelVar(NULL),
	vbo(NULL)
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
	comps.Append(VertexComponent((VertexComponent::SemanticName)0, 0, VertexComponent::Float2, 0));
	comps.Append(VertexComponent((VertexComponent::SemanticName)1, 0, VertexComponent::Float2, 0));
	comps.Append(VertexComponent((VertexComponent::SemanticName)2, 0, VertexComponent::Float4, 0));
	Ptr<MemoryVertexBufferLoader> vboLoader = MemoryVertexBufferLoader::Create();
	vboLoader->Setup(comps, MaxNumChars * 6, NULL, 0, VertexBuffer::UsageDynamic, VertexBuffer::AccessWrite, VertexBuffer::SyncingCoherent);

	// create vbo and load
	this->vbo = VertexBuffer::Create();
	this->vbo->SetLoader(vboLoader.upcast<ResourceLoader>());
	this->vbo->SetAsyncEnabled(false);
	this->vbo->Load();
	n_assert(this->vbo->IsLoaded());
	this->vbo->SetLoader(NULL);

	// map buffer
	this->vertexPtr = (byte*)this->vbo->Map(VertexBuffer::MapWrite);

	// setup primitive group
	this->group.SetNumIndices(0);
	this->group.SetBaseIndex(0);
	this->group.SetBaseVertex(0);

	// read font buffer
	this->ttf_buffer = n_new_array(unsigned char, 1 << 25);
#if __WIN32__
	// read font from windows
	fread(this->ttf_buffer, 1, 1 << 25, fopen("c:/windows/fonts/segoeui.ttf", "rb"));
#else
	fread(this->ttf_buffer, 1, 1 << 25, fopen("/usr/share/fonts/truetype/freefont/FreeSans.ttf", "rb"));
#endif
	this->bitmap = n_new_array(unsigned char, GLYPH_TEXTURE_SIZE*GLYPH_TEXTURE_SIZE);
	int charCount = 96;
	this->cdata = n_new_array(stbtt_packedchar, charCount);

	stbtt_pack_context context;
	if (!stbtt_PackBegin(&context, this->bitmap, GLYPH_TEXTURE_SIZE, GLYPH_TEXTURE_SIZE, 0, 1, NULL))
	{
		n_error("VkTextRenderer::Open(): Could not load font!");
	}
	stbtt_PackSetOversampling(&context, 4, 4);
	if (!stbtt_PackFontRange(&context, this->ttf_buffer, 0, 40.0f, 32, charCount, this->cdata))
	{
		n_error("VkTextRenderer::Open(): Could not load font!");
	}
	stbtt_PackEnd(&context);
	stbtt_InitFont(&this->font, this->ttf_buffer, 0);
	//stbtt_GetCodepointBitmap(&this->font, 0, stbtt_ScaleForPixelHeight(&this->font, 48.0f), )
	//stbtt_BakeFontBitmap(this->ttf_buffer, stbtt_GetFontOffsetForIndex(this->ttf_buffer, 0), 48.0f, bitmap, GLYPH_TEXTURE_SIZE, GLYPH_TEXTURE_SIZE, 32, 96, cdata);

	// setup random texture
	this->glyphTexture = ResourceManager::Instance()->CreateUnmanagedResource("GlyphTexture", Texture::RTTI).downcast<Texture>();
	Ptr<MemoryTextureLoader> loader = MemoryTextureLoader::Create();
	loader->SetImageBuffer(this->bitmap, GLYPH_TEXTURE_SIZE, GLYPH_TEXTURE_SIZE, PixelFormat::R8);
	this->glyphTexture->SetLoader(loader.upcast<ResourceLoader>());
	this->glyphTexture->SetResourceId("GlyphTexture");
	this->glyphTexture->SetAsyncEnabled(false);
	this->glyphTexture->Load();
	n_assert(this->glyphTexture->IsLoaded());
	this->glyphTexture->SetLoader(0);

	// create shader instance
	this->shader = ShaderServer::Instance()->CreateShaderState("shd:text", { NEBULAT_SYSTEM_GROUP });
	this->shader->SelectActiveVariation(ShaderServer::Instance()->FeatureStringToMask("Static"));

	// get variable
	this->texVar = this->shader->GetVariableByName("Texture");
	this->modelVar = this->shader->GetVariableByName("TextProjectionModel");

	n_delete_array(bitmap);
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
	this->shader->Discard();
	this->shader = 0;

	// unmap buffer
	this->vbo->Unmap();
	this->vertexPtr = 0;

	// discard vbo
	this->vbo->Unload();
	this->vbo = 0;

	// unload texture
	ResourceManager::Instance()->UnregisterUnmanagedResource(this->glyphTexture.upcast<Resource>());
	this->glyphTexture = 0;

	n_delete_array(ttf_buffer);
}

//------------------------------------------------------------------------------
/**
*/
void
VkTextRenderer::DrawTextElements()
{
	n_assert(this->IsOpen());

	// get display mode
	Ptr<CoreGraphics::Window> wnd = DisplayDevice::Instance()->GetCurrentWindow();
	const DisplayMode& displayMode = wnd->GetDisplayMode();

	// calculate projection matrix
	matrix44 proj = matrix44::orthooffcenterrh(0, (float)displayMode.GetWidth(), (float)displayMode.GetHeight(), 0, -1.0f, +1.0f);

	// apply shader
	this->shader->Apply();

	// update shader state
	this->texVar->SetTexture(this->glyphTexture);
	this->modelVar->SetMatrix(proj);

	uint screenWidth, screenHeight;
	screenWidth = displayMode.GetWidth();
	screenHeight = displayMode.GetHeight();

	// update vertex buffer
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
		stbtt_GetFontVMetrics(&this->font, &ascent, &descent, &gap);
		float scale = stbtt_ScaleForPixelHeight(&this->font, fontSize);
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
				stbtt_GetPackedQuad(this->cdata, GLYPH_TEXTURE_SIZE, GLYPH_TEXTURE_SIZE, *text - 32, &left, &top, &quad, 1);

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
				this->vertices[vert + 1].color = color;
				this->vertices[vert + 1].uv.x() = quad.s1;
				this->vertices[vert + 1].uv.y() = quad.t0;
				this->vertices[vert + 1].vertex.x() = pos1.z();
				this->vertices[vert + 1].vertex.y() = pos1.w();

				// vertex 3
				this->vertices[vert + 2].color = color;
				this->vertices[vert + 2].uv.x() = quad.s1;
				this->vertices[vert + 2].uv.y() = quad.t1;
				this->vertices[vert + 2].vertex.x() = pos2.x();
				this->vertices[vert + 2].vertex.y() = pos2.y();

				// vertex 4
				this->vertices[vert + 3].color = color;
				this->vertices[vert + 3].uv.x() = quad.s0;
				this->vertices[vert + 3].uv.y() = quad.t0;
				this->vertices[vert + 3].vertex.x() = pos2.z();
				this->vertices[vert + 3].vertex.y() = pos2.w();

				// vertex 5
				this->vertices[vert + 4].color = color;
				this->vertices[vert + 4].uv.x() = quad.s1;
				this->vertices[vert + 4].uv.y() = quad.t1;
				this->vertices[vert + 4].vertex.x() = pos3.x();
				this->vertices[vert + 4].vertex.y() = pos3.y();

				// vertex 6
				this->vertices[vert + 5].color = color;
				this->vertices[vert + 5].uv.x() = quad.s0;
				this->vertices[vert + 5].uv.y() = quad.t1;
				this->vertices[vert + 5].vertex.x() = pos3.z();
				this->vertices[vert + 5].vertex.y() = pos3.w();

				vert += 6;
			}

			n_assert(vert <= MaxNumChars * 6);
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
void
VkTextRenderer::Draw(TextElementVertex* buffer, SizeT numChars)
{
	// get render device
	RenderDevice* dev = RenderDevice::Instance();

	// copy to buffer
	memcpy(this->vertexPtr, buffer, sizeof(TextElementVertex) * numChars * 6);

	// create primitive group
	this->group.SetNumVertices(numChars * 6);

	// get render device and set it up
	dev->SetPrimitiveTopology(CoreGraphics::PrimitiveTopology::TriangleList);
	dev->SetVertexLayout(this->vbo->GetVertexLayout());
	dev->SetStreamVertexBuffer(0, this->vbo, 0);	
	dev->SetPrimitiveGroup(this->group);

	// set viewport
	Ptr<CoreGraphics::Window> wnd = DisplayDevice::Instance()->GetCurrentWindow();
	const DisplayMode& displayMode = wnd->GetDisplayMode();
	uint screenWidth, screenHeight;
	screenWidth = displayMode.GetWidth();
	screenHeight = displayMode.GetHeight();
	//dev->SetViewport(Math::rectangle<int>(0, 0, screenWidth, screenHeight), 0);
	//dev->SetScissorRect(Math::rectangle<int>(0, 0, screenWidth, screenHeight), 0);

	// commit changes
	this->shader->Commit();

	// setup device
	dev->Draw();
}

//------------------------------------------------------------------------------
/**
*/
Math::float2
VkTextRenderer::TransformTextVertex(const Math::float2& pos, const Math::float2& offset, const Math::float2& scale)
{
	return float2::multiply(float2::multiply(pos, offset), scale);
}

} // namespace Vulkan