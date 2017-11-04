#pragma once
//------------------------------------------------------------------------------
/**
	Implements a mesh loader from stream into Vulkan. Doubtful this is Vulkan specific...
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourcestreampool.h"
#include "coregraphics/base/gpuresourcebase.h"
namespace CoreGraphics
{
class MeshPool : public Resources::ResourceStreamPool
{
	__DeclareClass(MeshPool);
public:
	/// constructor
	MeshPool();
	/// destructor
	virtual ~MeshPool();

	/// set the intended resource usage (default is UsageImmutable)
	void SetUsage(Base::GpuResourceBase::Usage usage);
	/// get resource usage
	Base::GpuResourceBase::Usage GetUsage() const;
	/// set the intended resource access (default is AccessNone)
	void SetAccess(Base::GpuResourceBase::Access access);
	/// get the resource access
	Base::GpuResourceBase::Access GetAccess() const;

	/// bind mesh
	void BindMesh(const Resources::ResourceId id);
	/// bind primitive group for currently bound mesh
	void BindPrimitiveGroup(const IndexT primgroup);

private:
	
	/// perform load
	LoadStatus Load(const Ids::Id24 id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream);
	/// unload resource (overload to implement resource deallocation)
	void Unload(const Ids::Id24 id);

#if NEBULA3_LEGACY_SUPPORT
	/// setup mesh from nvx2 file in memory
	LoadStatus SetupMeshFromNvx2(const Ptr<IO::Stream>& stream, const Ids::Id24 res);
#endif
	/// setup mesh from nvx3 file in memory
	LoadStatus SetupMeshFromNvx3(const Ptr<IO::Stream>& stream, const Ids::Id24 res);
	/// setup mesh from n3d3 file in memory
	LoadStatus SetupMeshFromN3d3(const Ptr<IO::Stream>& stream, const Ids::Id24 res);

protected:
	Base::GpuResourceBase::Usage usage;
	Base::GpuResourceBase::Access access;

	struct Mesh
	{
		Resources::ResourceId vertexBuffer;
		Resources::ResourceId indexBuffer;
		Resources::ResourceId vertexLayout;
		CoreGraphics::PrimitiveTopology::Code topology;
		Util::Array<CoreGraphics::PrimitiveGroup> primitiveGroups;
	};
	Ids::Id24 activeMesh;

	Ids::IdAllocator<Mesh> allocator;
	__ImplementResourceAllocator(allocator);
};


//------------------------------------------------------------------------------
/**
*/
inline void
MeshPool::SetUsage(Base::GpuResourceBase::Usage usage_)
{
	this->usage = usage_;
}

//------------------------------------------------------------------------------
/**
*/
inline Base::GpuResourceBase::Usage
MeshPool::GetUsage() const
{
	return this->usage;
}

//------------------------------------------------------------------------------
/**
*/
inline void
MeshPool::SetAccess(Base::GpuResourceBase::Access access_)
{
	this->access = access_;
}

//------------------------------------------------------------------------------
/**
*/
inline Base::GpuResourceBase::Access
MeshPool::GetAccess() const
{
	return this->access;
}
} // namespace CoreGraphics