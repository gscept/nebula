#pragma once
//------------------------------------------------------------------------------
/**
	Implements a text renderer using Vulkan.
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/textrendererbase.h"
#include "coregraphics/shader.h"
#include "coregraphics/vertexbuffer.h"
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
	void DrawTextElements();

	static const int MaxNumChars = 65535;
private:


	/// draws text buffer
	void Draw(TextElementVertex* buffer, SizeT numChars);

	/// helper function which moves vertex into proper position
	Math::float2 TransformTextVertex(const Math::float2& pos, const Math::float2& offset, const Math::float2& scale);

	// define tff buffer
	unsigned char* ttf_buffer;
	stbtt_packedchar* cdata;
	stbtt_fontinfo font;
	unsigned char* bitmap;

	TextElementVertex vertices[MaxNumChars * 6];
	CoreGraphics::ResourceTableId textTable;
	CoreGraphics::ShaderProgramId program;
	IndexT texVar;
	CoreGraphics::ConstantBinding modelVar;
	CoreGraphics::TextureId glyphTexture;
	CoreGraphics::PrimitiveGroup group;
	CoreGraphics::VertexBufferId vbo;
	byte* vertexPtr;
};
} // namespace Vulkan