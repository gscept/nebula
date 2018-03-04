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
#include "coregraphics/shader.h"
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

	struct Instance : public TransformNode::Instance
	{
		CoreGraphics::ShaderId sharedShader;
		CoreGraphics::ShaderConstantId modelVar;
		CoreGraphics::ShaderConstantId invModelVar;
		CoreGraphics::ShaderConstantId modelViewProjVar;
		CoreGraphics::ShaderConstantId modelViewVar;
		CoreGraphics::ShaderConstantId objectIdVar;
		IndexT bufferIndex;
	};

protected:
	friend class StreamModelPool;

	/// load shader state
	bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader);


	Resources::ResourceName materialName;
};
} // namespace Models