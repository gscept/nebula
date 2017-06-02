#pragma once
//------------------------------------------------------------------------------
/**
	Executes compute shader.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/shaderstate.h"
namespace Frame
{
class FrameCompute : public FrameOp
{
	__DeclareClass(FrameCompute);
public:
	/// constructor
	FrameCompute();
	/// destructor
	virtual ~FrameCompute();

	/// set shader
	void SetShaderState(const Ptr<CoreGraphics::ShaderState>& state);
	/// set variation in shader
	void SetVariation(const CoreGraphics::ShaderFeature::Mask mask);
	/// set computation invocations
	void SetInvocations(const SizeT x, const SizeT y, const SizeT z);

	/// discard operation
	void Discard();
	/// run operation
	void Run(const IndexT frameIndex);
private:
	CoreGraphics::ShaderFeature::Mask mask;
	Ptr<CoreGraphics::ShaderState> state;
	SizeT x, y, z;
};

//------------------------------------------------------------------------------
/**
*/
inline void
FrameCompute::SetShaderState(const Ptr<CoreGraphics::ShaderState>& state)
{
	this->state = state;
}

//------------------------------------------------------------------------------
/**
*/
inline void
FrameCompute::SetVariation(const CoreGraphics::ShaderFeature::Mask mask)
{
	this->mask = mask;
}

//------------------------------------------------------------------------------
/**
*/
inline void
FrameCompute::SetInvocations(const SizeT x, const SizeT y, const SizeT z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

} // namespace Frame2