#pragma once
//------------------------------------------------------------------------------
/**
	A frame pass prepares a rendering sequence, draws and subpasses must reside
	within one of these objects.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "framesubpass.h"
#include "coregraphics/pass.h"
namespace Frame
{
class FramePass : public FrameOp
{
	__DeclareClass(FramePass);
public:
	/// constructor
	FramePass();
	/// destructor
	virtual ~FramePass();

	/// add subpass
	void AddSubpass(const Ptr<FrameSubpass>& subpass);

	/// get list of subpasses
	const Util::Array<Ptr<FrameSubpass>>& GetSubpasses() const;

	/// discard operation
	void Discard();
	/// run operation
	void Run(const IndexT frameIndex);
	/// handle display resizing
	void OnWindowResized();

	CoreGraphics::PassId pass;

private:
	Util::Array<Ptr<FrameSubpass>> subpasses;
};

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<Frame::FrameSubpass>>&
FramePass::GetSubpasses() const
{
	return this->subpasses;
}

} // namespace Frame2