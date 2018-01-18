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

	/// set pass
	void SetPass(const CoreGraphics::PassId pass);
	/// get pass
	const CoreGraphics::PassId GetPass() const;
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
private:
	CoreGraphics::PassId pass;
	Util::Array<Ptr<FrameSubpass>> subpasses;
};

//------------------------------------------------------------------------------
/**
*/
inline void
FramePass::SetPass(const CoreGraphics::PassId pass)
{
	this->pass = pass;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::PassId
FramePass::GetPass() const
{
	return this->pass;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<Frame::FrameSubpass>>&
FramePass::GetSubpasses() const
{
	return this->subpasses;
}

} // namespace Frame2