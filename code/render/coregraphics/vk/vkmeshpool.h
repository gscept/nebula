#pragma once
//------------------------------------------------------------------------------
/**
	Implements a mesh loader from stream into Vulkan. Doubtful this is Vulkan specific...
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourcestreampool.h"
#include "coregraphics/base/gpuresourcebase.h"
namespace Vulkan
{
class VkMeshPool : public Resources::ResourceStreamPool
{
	__DeclareClass(VkMeshPool);
public:
	/// constructor
	VkMeshPool();
	/// destructor
	virtual ~VkMeshPool();

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

#if NEBULA_LEGACY_SUPPORT
	/// setup mesh from nvx2 file in memory
	bool SetupMeshFromNvx2(const Ptr<IO::Stream>& stream, const Ptr<Resources::Resource>& res);
#endif
	/// setup mesh from nvx3 file in memory
	bool SetupMeshFromNvx3(const Ptr<IO::Stream>& stream, const Ptr<Resources::Resource>& res);
	/// setup mesh from n3d3 file in memory
	bool SetupMeshFromN3d3(const Ptr<IO::Stream>& stream, const Ptr<Resources::Resource>& res);

protected:
	Base::GpuResourceBase::Usage usage;
	Base::GpuResourceBase::Access access;
};


//------------------------------------------------------------------------------
/**
*/
inline void
VkMeshPool::SetUsage(Base::GpuResourceBase::Usage usage_)
{
	this->usage = usage_;
}

//------------------------------------------------------------------------------
/**
*/
inline Base::GpuResourceBase::Usage
VkMeshPool::GetUsage() const
{
	return this->usage;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkMeshPool::SetAccess(Base::GpuResourceBase::Access access_)
{
	this->access = access_;
}

//------------------------------------------------------------------------------
/**
*/
inline Base::GpuResourceBase::Access
VkMeshPool::GetAccess() const
{
	return this->access;
}
} // namespace Vulkan