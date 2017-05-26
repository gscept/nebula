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
	void SetUsage(Base::ResourceBase::Usage usage);
	/// get resource usage
	Base::ResourceBase::Usage GetUsage() const;
	/// set the intended resource access (default is AccessNone)
	void SetAccess(Base::ResourceBase::Access access);
	/// get the resource access
	Base::ResourceBase::Access GetAccess() const;

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
	Base::ResourceBase::Usage usage;
	Base::ResourceBase::Access access;
};


//------------------------------------------------------------------------------
/**
*/
inline void
VkStreamMeshLoader::SetUsage(Base::ResourceBase::Usage usage_)
{
	this->usage = usage_;
}

//------------------------------------------------------------------------------
/**
*/
inline Base::ResourceBase::Usage
VkStreamMeshLoader::GetUsage() const
{
	return this->usage;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkStreamMeshLoader::SetAccess(Base::ResourceBase::Access access_)
{
	this->access = access_;
}

//------------------------------------------------------------------------------
/**
*/
inline Base::ResourceBase::Access
VkStreamMeshLoader::GetAccess() const
{
	return this->access;
}
} // namespace Vulkan