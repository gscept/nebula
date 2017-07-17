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

protected:
	friend class ModelLoader;

	/// load shader state
	bool Load(const Util::FourCC& tag, const Ptr<Models::ModelLoader>& loader, const Ptr<IO::BinaryReader>& reader);

	Resources::ResourceName materialName;
};
} // namespace Models