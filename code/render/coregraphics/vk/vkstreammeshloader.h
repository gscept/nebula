#pragma once
//------------------------------------------------------------------------------
/**
	Implements a mesh loader from stream into Vulkan. Doubtful this is Vulkan specific...
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/streamresourceloader.h"
#include "coregraphics/base/resourcebase.h"
namespace Vulkan
{
class VkStreamMeshLoader : public Resources::StreamResourceLoader
{
	__DeclareClass(VkStreamMeshLoader);
public:
	/// constructor
	VkStreamMeshLoader();
	/// destructor
	virtual ~VkStreamMeshLoader();

	/// set the intended resource usage (default is UsageImmutable)
	void SetUsage(Base::GpuResourceBase::Usage usage);
	/// get resource usage
	Base::GpuResourceBase::Usage GetUsage() const;
	/// set the intended resource access (default is AccessNone)
	void SetAccess(Base::GpuResourceBase::Access access);
	/// get the resource access
	Base::GpuResourceBase::Access GetAccess() const;

private:
	/// setup mesh from generic stream, branches to specialized loader methods
	virtual bool SetupResourceFromStream(const Ptr<IO::Stream>& stream);
#if NEBULA3_LEGACY_SUPPORT
	/// setup mesh from nvx2 file in memory
	bool SetupMeshFromNvx2(const Ptr<IO::Stream>& stream);
#endif
	/// setup mesh from nvx3 file in memory
	bool SetupMeshFromNvx3(const Ptr<IO::Stream>& stream);
	/// setup mesh from n3d3 file in memory
	bool SetupMeshFromN3d3(const Ptr<IO::Stream>& stream);

protected:
	Base::GpuResourceBase::Usage usage;
	Base::GpuResourceBase::Access access;
};


//------------------------------------------------------------------------------
/**
*/
inline void
VkStreamMeshLoader::SetUsage(Base::GpuResourceBase::Usage usage_)
{
	this->usage = usage_;
}

//------------------------------------------------------------------------------
/**
*/
inline Base::GpuResourceBase::Usage
VkStreamMeshLoader::GetUsage() const
{
	return this->usage;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkStreamMeshLoader::SetAccess(Base::GpuResourceBase::Access access_)
{
	this->access = access_;
}

//------------------------------------------------------------------------------
/**
*/
inline Base::GpuResourceBase::Access
VkStreamMeshLoader::GetAccess() const
{
	return this->access;
}
} // namespace Vulkan