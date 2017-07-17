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
	__DeclareClass(PrimitiveNode);
public:
	/// constructor
	PrimitiveNode();
	/// destructor
	virtual ~PrimitiveNode();

protected:
	friend class ModelLoader;

	/// load primitive
	virtual bool Load(const Util::FourCC& tag, const Ptr<Models::ModelLoader>& loader, const Ptr<IO::BinaryReader>& reader);

	Resources::ResourceName meshName;
	uint32_t primitiveGroupIndex;
};
} // namespace Models