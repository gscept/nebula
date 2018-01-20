#pragma once
//------------------------------------------------------------------------------
/**
	Implements a mesh loader from stream into Vulkan. Doubtful this is Vulkan specific...
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourcestreampool.h"
#include "coregraphics/gpubuffertypes.h"
#include "coregraphics/mesh.h"
namespace CoreGraphics
{
class StreamMeshPool : public Resources::ResourceStreamPool
{
	__DeclareClass(StreamMeshPool);
public:
	/// constructor
	StreamMeshPool();
	/// destructor
	virtual ~StreamMeshPool();

	/// set the intended resource usage (default is UsageImmutable)
	void SetUsage(GpuBufferTypes::Usage usage);
	/// get resource usage
	GpuBufferTypes::Usage GetUsage() const;
	/// set the intended resource access (default is AccessNone)
	void SetAccess(GpuBufferTypes::Access access);
	/// get the resource access
	GpuBufferTypes::Access GetAccess() const;

	/// bind mesh
	void MeshBind(const Resources::ResourceId id);
	/// bind primitive group for currently bound mesh
	void BindPrimitiveGroup(const IndexT primgroup);

private:
	
	/// perform load
	LoadStatus LoadFromStream(const Ids::Id24 id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream) override;
	/// unload resource (overload to implement resource deallocation)
	void Unload(const Ids::Id24 id) override;

	/// allocate object
	Resources::ResourceUnknownId AllocObject() override;
	/// deallocate object
	void DeallocObject(const Resources::ResourceUnknownId id) override;

#if NEBULA3_LEGACY_SUPPORT
	/// setup mesh from nvx2 file in memory
	LoadStatus SetupMeshFromNvx2(const Ptr<IO::Stream>& stream, const Ids::Id24 res);
#endif
	/// setup mesh from nvx3 file in memory
	LoadStatus SetupMeshFromNvx3(const Ptr<IO::Stream>& stream, const Ids::Id24 res);
	/// setup mesh from n3d3 file in memory
	LoadStatus SetupMeshFromN3d3(const Ptr<IO::Stream>& stream, const Ids::Id24 res);

protected:
	GpuBufferTypes::Usage usage;
	GpuBufferTypes::Access access;

	Resources::ResourceId activeMesh;
};

//------------------------------------------------------------------------------
/**
*/
inline void
StreamMeshPool::SetUsage(GpuBufferTypes::Usage usage_)
{
	this->usage = usage_;
}

//------------------------------------------------------------------------------
/**
*/
inline GpuBufferTypes::Usage
StreamMeshPool::GetUsage() const
{
	return this->usage;
}

//------------------------------------------------------------------------------
/**
*/
inline void
StreamMeshPool::SetAccess(GpuBufferTypes::Access access_)
{
	this->access = access_;
}

//------------------------------------------------------------------------------
/**
*/
inline GpuBufferTypes::Access
StreamMeshPool::GetAccess() const
{
	return this->access;
}
} // namespace CoreGraphics