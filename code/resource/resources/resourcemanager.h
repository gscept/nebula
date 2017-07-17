#pragma once
//------------------------------------------------------------------------------
/**
	Description
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <functional>
#include "core/refcounted.h"
#include "ids/id.h"
#include "core/singleton.h"
#include "resourcecontainer.h"
#include "resourceid.h"
#include "resourceloader.h"
#include "resourceloaderthread.h"
namespace Resources
{
class ResourceManager : public Core::RefCounted
{
	__DeclareClass(ResourceManager);
	__DeclareSingleton(ResourceManager);
public:
	/// constructor
	ResourceManager();
	/// destructor
	virtual ~ResourceManager();

	/// open manager
	void Open();
	/// close manager
	void Close();

	/// enter lock-step mode, during this phase, resources may not be discarded
	void EnterLockstep();
	/// update resource manager, call each frame
	void Update(IndexT frameIndex);
	/// exit lock-step mode
	void ExitLockstep();

	/// Create a new resource, which will be loaded at some later point, if not already loaded
	Resources::ResourceId CreateResource(const ResourceName& id, std::function<void(const Resources::ResourceId)> success = nullptr, std::function<void(const Resources::ResourceId)> failed = nullptr, bool immediate = false);
	/// overload which also takes an identifying tag, which is used to group-discard resources
	Resources::ResourceId CreateResource(const ResourceName& id, const Util::StringAtom& tag, std::function<void(const Resources::ResourceId)> success = nullptr, std::function<void(const Resources::ResourceId)> failed = nullptr, bool immediate = false);
	/// discard resource
	void DiscardResource(const Resources::ResourceId res);
	/// discard all resources by tag
	void DiscardResources(const Util::StringAtom& tag);
	/// reserve resource (for self-managed resources)
	Resources::ResourceId ReserveResource(const ResourceName& id, const Util::StringAtom& tag, const Core::Rtti& type);

	/// get type of resource (i.e RTTI used by ResourceLoader)
	Core::Rtti* GetType(const Resources::ResourceId id);
	/// get internal resource 
	template <class TYPE>
	const Ptr<TYPE>& GetResource(const Resources::ResourceId id);

	/// register a file loader, which takes an extension and the RTTI of the resource type to create
	void RegisterFileLoader(const Util::StringAtom& ext, const Core::Rtti& loaderClass);
	/// register a memory loader, which maps only a resource type (RTTI)
	void RegisterLoader(const Core::Rtti& loaderClass);
private:
	friend class ResourceLoader;

	bool open;
	Ptr<ResourceLoaderThread> loaderThread;
	Util::Dictionary<Util::StringAtom, IndexT> extensionMap;
	Util::Dictionary<const Core::Rtti*, IndexT> typeMap;
	Util::Array<Ptr<ResourceLoader>> loaders;
	//Util::Dictionary<Util::StringAtom, Ptr<ResourceLoader>> loaders;

	static int32_t UniqueLoaderCounter;
};

//------------------------------------------------------------------------------
/**
	If a previous call to CreateResources triggered a resource load, and this evocation enforces the resource loading to be immediate, then despite
	this call not actually triggering a resource to be loaded, the referenced resource will be loaded immediately nonetheless.
*/
inline Resources::ResourceId
Resources::ResourceManager::CreateResource(const ResourceName& id, std::function<void(const Resources::ResourceId)> success, std::function<void(const Resources::ResourceId)> failed, bool immediate)
{
	return this->CreateResource(id, "", success, failed, immediate);
}

//------------------------------------------------------------------------------
/**
	If a previous call to CreateResources triggered a resource load, and this evocation enforces the resource loading to be immediate, then despite
	this call not actually triggering a resource to be loaded, the referenced resource will be loaded immediately nonetheless.
*/
inline Resources::ResourceId
Resources::ResourceManager::CreateResource(const ResourceName& res, const Util::StringAtom& tag, std::function<void(const Resources::ResourceId)> success, std::function<void(const Resources::ResourceId)> failed, bool immediate)
{
	// get resource loader by extension
	Util::String ext = res.AsString().GetFileExtension();
	IndexT i = this->extensionMap.FindIndex(ext);
	n_assert_fmt(i != InvalidIndex, "No resource loader is associated with file extension '%s'", ext.AsCharPtr());
	const Ptr<ResourceLoader>& loader = this->loaders[this->extensionMap.ValueAtIndex(i)];

	// create container and cast to actual resource type
	Resources::ResourceId id = loader->CreateResource(res, tag, success, failed, immediate);
	return id;
}

//------------------------------------------------------------------------------
/**
	Discards a single resource, and removes the callbacks to it from
*/
inline void
Resources::ResourceManager::DiscardResource(const Resources::ResourceId id)
{
	// get id of loader
	const Ids::Id8 loaderid = Ids::Id::GetTiny(Ids::Id::GetLow(id));

	// get resource loader by extension
	n_assert(this->loaders.Size() > loaderid);
	const Ptr<ResourceLoader>& loader = this->loaders[loaderid];

	// discard container
	loader->DiscardResource(id);
}

//------------------------------------------------------------------------------
/**
*/
inline Resources::ResourceId
ResourceManager::ReserveResource(const ResourceName& id, const Util::StringAtom& tag, const Core::Rtti& type)
{
	const Ptr<ResourceLoader>& loader = this->loaders[this->typeMap[&type]];
	return loader->ReserveResource(id, tag);
}

//------------------------------------------------------------------------------
/**
*/
inline Resources::ResourceId
CreateResource(const ResourceName& res, const Util::StringAtom& tag, std::function<void(const Resources::ResourceId)> success = nullptr, std::function<void(const Resources::ResourceId)> failed = nullptr, bool immediate = false)
{
	return ResourceManager::Instance()->CreateResource(res, tag, success, failed, immediate);
}

//------------------------------------------------------------------------------
/**
*/
inline void
DiscardResource(const Resources::ResourceId id)
{
	ResourceManager::Instance()->DiscardResource(id);
}

//------------------------------------------------------------------------------
/**
*/
inline Resources::ResourceId
ReserveResource(const ResourceName& res, const Util::StringAtom& tag, const Core::Rtti& type)
{
	return ResourceManager::Instance()->ReserveResource(res, tag, type);
}

//------------------------------------------------------------------------------
/**
*/
inline Core::Rtti*
GetType(const Resources::ResourceId id)
{
	ResourceManager::Instance()->GetType(id);
}

//------------------------------------------------------------------------------
/**
*/
template <class TYPE>
inline const Ptr<TYPE>&
GetResource(const Resources::ResourceId id)
{
	static_assert(std::is_base_of<Resource, TYPE>::value, "Template argument is not a Resource type!");
	return ResourceManager::Instance()->GetResource<TYPE>(id);
}

} // namespace Resources