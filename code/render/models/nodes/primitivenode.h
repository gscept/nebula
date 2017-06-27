#pragma once
//------------------------------------------------------------------------------
/**
	A primitive node contains a mesh resource and a primitive group id.

	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "modelnode.h"
namespace Models
{
class PrimitiveNode : public ModelNode
{
	__DeclareClass(PrimitiveNode);
public:
	/// constructor
	PrimitiveNode();
	/// destructor
	virtual ~PrimitiveNode();
private:
};
} // namespace Models