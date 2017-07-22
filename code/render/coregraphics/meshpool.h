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

private:
	
	/// perform load
	LoadStatus Load(const Ptr<Resources::Resource>& res, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream);

#if NEBULA3_LEGACY_SUPPORT
	/// setup mesh from nvx2 file in memory
	LoadStatus SetupMeshFromNvx2(const Ptr<IO::Stream>& stream, const Ptr<Resources::Resource>& res);
#endif
	/// setup mesh from nvx3 file in memory
	LoadStatus SetupMeshFromNvx3(const Ptr<IO::Stream>& stream, const Ptr<Resources::Resource>& res);
	/// setup mesh from n3d3 file in memory
	LoadStatus SetupMeshFromN3d3(const Ptr<IO::Stream>& stream, const Ptr<Resources::Resource>& res);

protected:
	Base::GpuResourceBase::Usage usage;
	Base::GpuResourceBase::Access access;
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