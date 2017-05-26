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
namespace Frame2
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
	void SetPass(const Ptr<CoreGraphics::Pass>& pass);
	/// get pass
	const Ptr<CoreGraphics::Pass>& GetPass() const;
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
	Ptr<CoreGraphics::Pass> pass;
	Util::Array<Ptr<FrameSubpass>> subpasses;
};

//------------------------------------------------------------------------------
/**
*/
inline void
FramePass::SetPass(const Ptr<CoreGraphics::Pass>& pass)
{
	this->pass = pass;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::Pass>&
FramePass::GetPass() const
{
	return this->pass;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<Frame2::FrameSubpass>>&
FramePass::GetSubpasses() const
{
	return this->subpasses;
}

} // namespace Frame2