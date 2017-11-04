#pragma once
//------------------------------------------------------------------------------
/**
	The shader state node wraps the shader associated with a certain primitive node,
	or group of primitive nodes.
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "modelnode.h"
#include "resources/resourceid.h"
#include "transformnode.h"
namespace Models
{
class ShaderStateNode : public TransformNode
{
public:
	/// constructor
	ShaderStateNode();
	/// destructor
	virtual ~ShaderStateNode();

protected:
	friend class ModelPool;

	/// load shader state
	bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader);

	struct Instance : public TransformNode::Instance
	{
		CoreGraphics::ShaderId sharedShader;
		CoreGraphics::ShaderVariableId modelVar;
		CoreGraphics::ShaderVariableId invModelVar;
		CoreGraphics::ShaderVariableId modelViewProjVar;
		CoreGraphics::ShaderVariableId modelViewVar;
		CoreGraphics::ShaderVariableId objectIdVar;
		IndexT bufferIndex;
	};

	Resources::ResourceName materialName;
};
} // namespace Models