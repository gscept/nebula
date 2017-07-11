#pragma once
//------------------------------------------------------------------------------
/**
	Description
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <functional>
#include "core/refcounted.h"
#include "core/id.h"
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

	/// update resource manager, call each frame
	void Update(IndexT frameIndex);

	/// create a new resource, which will be loaded at some later point, if not already loaded
	Core::Id CreateResource(const ResourceId& id, std::function<void(const Core::Id&)> success = nullptr, std::function<void(const Core::Id&)> failed = nullptr, bool immediate = false);
	/// overload which also takes an identifying tag, which is used to group-discard resources
	Core::Id CreateResource(const ResourceId& id, const Util::StringAtom& tag, std::function<void(const Core::Id&)> success = nullptr, std::function<void(const Core::Id&)> failed = nullptr, bool immediate = false);
	/// discard resource
	void DiscardResource(const Core::Id& res);
	/// discard all resources by tag
	void DiscardResources(const Util::StringAtom& tag);

	/// register a resource loader by using RTTI, this function will also create the loader
	void RegisterLoader(const Util::StringAtom& ext, const Core::Rtti& loaderClass);
private:
	friend class ResourceLoader;

	bool open;
	Ptr<ResourceLoaderThread> loaderThread;
	Util::Dictionary<Util::StringAtom, IndexT> extensionMap;
	Util::Array<Ptr<ResourceLoader>> loaders;
	//Util::Dictionary<Util::StringAtom, Ptr<ResourceLoader>> loaders;

	static int32_t UniqueLoaderCounter;
};

//------------------------------------------------------------------------------
/**
*/
inline Core::Id
Resources::ResourceManager::CreateResource(const ResourceId& id, std::function<void(const Core::Id&)> success, std::function<void(const Core::Id&)> failed, bool immediate)
{
	return this->CreateResource(id, "", success, failed, immediate);
}

//------------------------------------------------------------------------------
/**
*/
inline Core::Id
Resources::ResourceManager::CreateResource(const ResourceId& res, const Util::StringAtom& tag, std::function<void(const Core::Id&)> success, std::function<void(const Core::Id&)> failed, bool immediate)
{
	// get resource loader by extension
	Util::String ext = res.AsString().GetFileExtension();
	IndexT i = this->extensionMap.FindIndex(ext);
	n_assert_fmt(i != InvalidIndex, "No resource loader is associated with file extension '%s'", ext.AsCharPtr());
	const Ptr<ResourceLoader>& loader = this->loaders[this->extensionMap.ValueAtIndex(i)];

	// create container and cast to actual resource type
	Core::Id id = loader->CreateResource(res, tag, success, failed, immediate);
	return id;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Resources::ResourceManager::DiscardResource(const Core::Id& res)
{
	// get id of loader
	const uchar loaderid = Core::Id::GetTiny(Core::Id::GetLow(res));

	// get resource loader by extension
	n_assert(this->loaders.Size() > loaderid);
	const Ptr<ResourceLoader>& loader = this->loaders[loaderid];

	// discard container
	loader->DiscardResource(res);
}

} // namespace Resources