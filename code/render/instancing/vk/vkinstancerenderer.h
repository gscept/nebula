#pragma once
//------------------------------------------------------------------------------
/**
	Implements a Vulkan specific renderer for instanced draws.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "instancing/base/instancerendererbase.h"
#include "coregraphics/constantbuffer.h"

namespace Vulkan
{
class VkInstanceRenderer : public Base::InstanceRendererBase
{
	__DeclareClass(VkInstanceRenderer);
public:
	/// constructor
	VkInstanceRenderer();
	/// destructor
	virtual ~VkInstanceRenderer();

	/// setup renderer
	void Setup();
	/// close rendered
	void Close();

	/// render
	void Render(const SizeT multiplier);
private:
	Ptr<CoreGraphics::ShaderState> shaderState;
	Ptr<CoreGraphics::ConstantBuffer> instancingBuffer;
	Ptr<CoreGraphics::ShaderVariable> instancingBlockVar;

	Ptr<CoreGraphics::ShaderVariable> modelArrayVar;
	Ptr<CoreGraphics::ShaderVariable> modelViewArrayVar;
	Ptr<CoreGraphics::ShaderVariable> modelViewProjectionArrayVar;
	Ptr<CoreGraphics::ShaderVariable> idArrayVar;

	static const int MaxInstancesPerBatch = 256;
};
} // namespace Vulkan