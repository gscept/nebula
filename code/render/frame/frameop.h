#pragma once
//------------------------------------------------------------------------------
/**
	A frame op is a base class for frame operations, use as base class for runnable
	sequences within a frame script.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/stringatom.h"
namespace Frame2
{
class FrameOp : public Core::RefCounted
{
	__DeclareClass(FrameOp);
public:
	typedef uint ExecutionMask;

	/// constructor
	FrameOp();
	/// destructor
	virtual ~FrameOp();

	/// set name
	void SetName(const Util::StringAtom& name);
	/// get name
	const Util::StringAtom& GetName() const;

	/// setup operation
	virtual void Setup();
	/// discard operation
	virtual void Discard();
	/// run operation
	virtual void Run(const IndexT frameIndex);
	/// run segment
	virtual void RunSegment(const FrameOp::ExecutionMask mask, const IndexT frameIndex);
	/// handle display resizing
	virtual void OnWindowResized();
protected:
	Util::StringAtom name;
};

//------------------------------------------------------------------------------
/**
*/
inline void
FrameOp::SetName(const Util::StringAtom& name)
{
	this->name = name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
FrameOp::GetName() const
{
	return this->name;
}

} // namespace Frame2