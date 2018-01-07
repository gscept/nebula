#pragma once
//------------------------------------------------------------------------------
/**
	A primitive node contains a mesh resource and a primitive group id.

	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "modelnode.h"
#include "math/bbox.h"
#include "coregraphics/primitivegroup.h"
#include "resources/resourceid.h"
#include "shaderstatenode.h"
namespace Models
{
class PrimitiveNode : public ShaderStateNode
{
public:
	/// constructor
	PrimitiveNode();
	/// destructor
	virtual ~PrimitiveNode();

protected:
	friend class StreamModelPool;

	/// load primitive
	virtual bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader);

	struct Instance : public ShaderStateNode::Instance
	{
		// empty
	};

	Resources::ResourceName meshName;
	Resources::ResourceId res;
	uint32_t primitiveGroupIndex;
};
} // namespace Models