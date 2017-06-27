#pragma once
//------------------------------------------------------------------------------
/**
	The shader state node wraps the shader associated with a certain primitive node,
	or group of primitive nodes.
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "modelnode.h"
namespace Models
{
class ShaderStateNode : public ModelNode
{
	__DeclareClass(ShaderStateNode);
public:
	/// constructor
	ShaderStateNode();
	/// destructor
	virtual ~ShaderStateNode();
private:
};
} // namespace Models