#pragma once
//------------------------------------------------------------------------------
/**
	The model server keeps track of all currently registered model resources, 
	and their bindings with shaders.

	TODO: 
	Ideally, all nodes should be allocated in a flat hierarchy according to the model.
	This way, updating all nodes with a root transform is a linear operation over all nodes.
	However, it is also useful to have two structures, one for run-time updates of nodes,
	and one for runtime modifications, like shader stuff. Also, move all of that work to
	the ModelContext for christs sakes!
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "materials/surfaceinstance.h"
#include "memory/sliceallocatorpool.h"
#include "ids/idgenerationpool.h"

namespace Models
{

ID_32_TYPE(ModelId);
ID_32_TYPE(ModelNodeId);

class ModelServer : public Core::RefCounted
{
	__DeclareClass(ModelServer);
	__DeclareSingleton(ModelServer);
public:
	/// constructor
	ModelServer();
	/// destructor
	virtual ~ModelServer();

private:

	/// the database is as such, surface name -> (mesh resource id -> primitive group)
	using SurfaceMeshInstanceDatabase = Util::Dictionary<
		Materials::SurfaceName::Code, Util::Dictionary<
		Resources::ResourceId, Util::Array<ModelNodeId>
		>>;

	SurfaceMeshInstanceDatabase database;
};

} // namespace Models