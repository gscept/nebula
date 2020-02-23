//------------------------------------------------------------------------------
// vktextrenderer.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vktextrenderer.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/displaydevice.h"
#include "threading/thread.h"
#include "io/ioserver.h"

#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb_truetype.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/memorytexturepool.h"
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
	comps.Append(VertexComponent((VertexComponent::SemanticName)0, 0, VertexComponent::Float2, 0));
	comps.Append(VertexComponent((VertexComponent::SemanticName)1, 0, VertexComponent::Float2, 0));
	comps.Append(VertexComponent((VertexComponent::SemanticName)2, 0, VertexComponent::Float4, 0));

	VertexBufferCreateInfo vboInfo = 
	{
		"TextRenderer VBO"_atm,
		GpuBufferTypes::AccessWrite, GpuBufferTypes::UsageDynamic, GpuBufferTypes::SyncingAutomatic,
		MaxNumChars * 6, comps,
		nullptr, 0
	};
	this->vbo = CreateVertexBuffer(vboInfo);
	this->vertexPtr = (byte*)VertexBufferMap(this->vbo, GpuBufferTypes::MapWrite);

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

	// load font
	Ptr<IO::Stream> fontStream = IO::IoServer::Instance()->CreateStream(fontPath);
	fontStream->SetAccessMode(IO::Stream::ReadAccess);
	if (fontStream->Open())
	{
		this->ttf_buffer = n_new_array(unsigned char, fontStream->GetSize());
		void* buf = fontStream->Map();
		memcpy(this->ttf_buffer, buf, fontStream->GetSize());
		fontStream->Unmap();
		fontStream->Close();
	}
	else
	{
		n_error("Failed to load font %s!", fontPath);
	}

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

	// setup random texture
	TextureCreateInfo texInfo;
	texInfo.name = "GlyphTexture"_atm;
	texInfo.usage = TextureUsage::ImmutableUsage;
	texInfo.tag = "render_system"_atm;
	texInfo.buffer = this->bitmap;
	texInfo.type = TextureType::Texture2D;
	texInfo.format = PixelFormat::R8;
	texInfo.width = GLYPH_TEXTURE_SIZE;
	texInfo.height = GLYPH_TEXTURE_SIZE;
	this->glyphTexture = CreateTexture(texInfo);

	// create shader instance
	const ShaderId shd = ShaderServer::Instance()->GetShader("shd:text.fxb");
	this->program = ShaderGetProgram(shd, ShaderServer::Instance()->FeatureStringToMask("Static"));
	this->textTable = ShaderCreateResourceTable(shd, NEBULA_BATCH_GROUP);
	// get variable

	this->texVar = ShaderGetResourceSlot(shd, "Texture");
	ResourceTableSetTexture(this->textTable, { this->glyphTexture, this->texVar, 0, CoreGraphics::SamplerId::Invalid(), false});
	ResourceTableCommitChanges(this->textTable);
	this->modelVar = ShaderGetConstantBinding(shd, "TextProjectionModel");

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
	VertexBufferUnmap(this->vbo);
	this->vertexPtr = nullptr;

	DestroyVertexBuffer(this->vbo);
	DestroyTexture(this->glyphTexture);

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
	CoreGraphics::WindowId wnd = DisplayDevice::Instance()->GetCurrentWindow();
	const DisplayMode& displayMode = WindowGetDisplayMode(wnd);

	// calculate projection matrix
	matrix44 proj = matrix44::orthooffcenterrh(0, (float)displayMode.GetWidth(), (float)displayMode.GetHeight(), 0, -1.0f, +1.0f);

	// apply shader and apply state
	CoreGraphics::SetShaderProgram(this->program);
	CoreGraphics::PushConstants(CoreGraphics::GraphicsPipeline, this->modelVar, sizeof(proj), (byte*)&proj);
	CoreGraphics::SetResourceTable(this->textTable, NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);

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
	// copy to buffer
	memcpy(this->vertexPtr, buffer, sizeof(TextElementVertex) * numChars * 6);

	// create primitive group
	this->group.SetNumVertices(numChars * 6);

	// get render device and set it up
	CoreGraphics::SetPrimitiveTopology(CoreGraphics::PrimitiveTopology::TriangleList);
	CoreGraphics::SetStreamVertexBuffer(0, this->vbo, 0);
	CoreGraphics::SetPrimitiveGroup(this->group);

	// set viewport
	CoreGraphics::WindowId wnd = DisplayDevice::Instance()->GetCurrentWindow();
	const DisplayMode& displayMode = WindowGetDisplayMode(wnd);
	uint screenWidth, screenHeight;
	screenWidth = displayMode.GetWidth();
	screenHeight = displayMode.GetHeight();
	//dev->SetViewport(Math::rectangle<int>(0, 0, screenWidth, screenHeight), 0);
	//dev->SetScissorRect(Math::rectangle<int>(0, 0, screenWidth, screenHeight), 0);

	// setup device
	CoreGraphics::Draw();
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