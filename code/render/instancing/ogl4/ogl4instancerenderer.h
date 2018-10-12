#pragma once
//------------------------------------------------------------------------------
/**
    @class Instancing::OGL4InstanceRenderer
    
    Performs instanced rendering with OpenGL 4
    
    (C) 2013 Gustav Sterbrant
	(C) 2013-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "instancing/base/instancerendererbase.h"
namespace Instancing
{
class OGL4InstanceRenderer : public InstanceRendererBase
{
	__DeclareClass(OGL4InstanceRenderer);
public:
	/// constructor
	OGL4InstanceRenderer();
	/// destructor
	virtual ~OGL4InstanceRenderer();

	/// setup renderer
	void Setup();
	/// close rendered
	void Close();

	/// render
	void Render(const SizeT multiplier);

private:
	Ptr<CoreGraphics::ConstantBuffer> instancingBuffer;
	Ptr<CoreGraphics::ShaderVariable> instancingBlockVar;

	Ptr<CoreGraphics::ShaderVariable> modelArrayVar;
	Ptr<CoreGraphics::ShaderVariable> modelViewArrayVar;
	Ptr<CoreGraphics::ShaderVariable> modelViewProjectionArrayVar;
	Ptr<CoreGraphics::ShaderVariable> idArrayVar;

	static const int MaxInstancesPerBatch = 256;
}; 
} // namespace Instancing
//------------------------------------------------------------------------------