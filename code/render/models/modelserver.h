#pragma once
//------------------------------------------------------------------------------
/**
	The model server keeps track of all currently registered model resources, 
	and their bindings with shaders
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "materials/surfaceinstance.h"
#include "memory/sliceallocatorpool.h"

namespace Models
{
class ModelServer : public Core::RefCounted
{
	__DeclareClass(ModelServer);
	__DeclareSingleton(ModelServer);
public:
	/// constructor
	ModelServer();
	/// destructor
	virtual ~ModelServer();

	typedef IndexT NodeInstanceId;
	struct NodeInstance
	{
		NodeInstanceId id;
		IndexT primitiveGroupId;
		Ptr<Materials::SurfaceInstance> surface; 
	};

	/// allocate a new (empty) node instance
	NodeInstance* CreateNodeInstance();

private:

	/// the database is as such, surface name -> (mesh resource id -> primitive group)
	using SurfaceMeshInstanceDatabase = Util::Dictionary<
		Materials::SurfaceName::Code, Util::Dictionary<
		Resources::ResourceId, Util::Dictionary<
		NodeInstanceId, NodeInstance>
		>>;

	Memory::SliceAllocatorPool<NodeInstance, 256, false> nodeInstancePool;
	SurfaceMeshInstanceDatabase database;
};
} // namespace Models