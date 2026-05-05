//------------------------------------------------------------------------------
// resourceserver.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "resourceserver.h"
#include "profiling/profiling.h"
#include "db/sqlite3/sqlite3factory.h"
#include "io/ioserver.h"
#include "attr/attribute.h"

#if NEBULA_DEBUG
#include "core/sysfunc.h"
#endif

namespace Attr
{
DefineAttrInt(URNHash, 'FURN', Attr::ReadWrite);
DefineAttrString(Export, 'FEXP', Attr::ReadWrite);
DefineAttrString(Work, 'FWOR', Attr::ReadWrite);
}


namespace Resources
{

__ImplementClass(Resources::ResourceServer, 'RMGR', Core::RefCounted);
__ImplementInterfaceSingleton(Resources::ResourceServer);

int32_t ResourceServer::UniquePoolCounter = 0;
//------------------------------------------------------------------------------
/**
*/
ResourceServer::ResourceServer()
{
    __ConstructSingleton;
    this->open = false;
}

//------------------------------------------------------------------------------
/**
*/
ResourceServer::~ResourceServer()
{
    __DestructSingleton;
    n_assert(!this->open); // make sure to call close before destroying the object
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceServer::Open()
{
    n_assert(!this->open);
    this->loaders.Reserve(256); // lower 8 bits of resource id can only get to 256
    this->open = true;
    UniquePoolCounter = 0;

    // Read the resource table and populate URN -> URI lookup map
    IO::URI resTableUri("export:resource_mapping.sqlite");

    if (this->dbFactory == nullptr)
    {
        this->dbFactory = Db::Sqlite3Factory::Create();
    }
    this->database = Db::DbFactory::Instance()->CreateDatabase();

    // Determine access mode
    IO::IoServer* ioServer = IO::IoServer::Instance();
    Db::Database::AccessMode accessMode = Db::Database::ReadWriteCreate;

    this->database->SetURI(resTableUri);
    this->database->SetAccessMode(accessMode);
    this->database->SetIgnoreUnknownColumns(false);

    // Use memory database if we're not in editor as it can modify the database
#if WITH_NEBULA_EDITOR == 0
    this->database->SetInMemoryDatabase(true);
#endif

    // Try to open database
    if (!this->database->Open())
    {
        n_printf("Failed to open resource lookup database at %s\n", resTableUri.AsString().AsCharPtr());
    }
    else
    {
        if (!this->database->HasTable("Mappings"))
        {
            // Close database, it's invalid
            this->database = nullptr;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceServer::Close()
{
    n_assert(this->open);

#if NEBULA_DEBUG
    // report any resources which have not been unloaded
    bool hasLeaks = false;
    Core::SysFunc::DebugOut("\n\n******** NEBULA RESOURCE MANAGER ********\n Beginning of resource leak report:");
    for (IndexT i = 0; i < this->loaders.Size(); i++)
    {
        Ptr<ResourceLoader> pool = this->loaders[i];
        if (!pool.isvalid())
        {
            continue;
        }
        for (auto & kvp : pool->ids)
        {
            const auto resource = pool->resources[kvp.Value()];
            if (pool->GetUsage(kvp.Value()) != 0)
            {
                const Resources::ResourceName& name = kvp.Key();
                Util::String msg = Util::String::Sprintf("Resource <%s> (id %d) from pool %d is not unloaded, usage is '%d'\n", name.Value(), resource.resourceId, pool->uniqueId, pool->GetUsage(kvp.Value()));
                Core::SysFunc::DebugOut(msg.AsCharPtr());
                hasLeaks = true;
            }
        }       
    }

    if (!hasLeaks)
    {
        Util::String msg = Util::String::Sprintf("\n\n******** NEBULA RESOURCE MANAGER ********\n All resources were properly freed!\n\n");
        Core::SysFunc::DebugOut(msg.AsCharPtr());
    }

#endif
    this->loaders.Clear();
    this->extensionMap.Clear();
    this->open = false;
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceServer::RegisterStreamLoader(const Util::StringAtom& ext, const Core::Rtti& loaderClass)
{
    n_assert(this->open);
    n_assert(loaderClass.IsDerivedFrom(ResourceLoader::RTTI));
    void* obj = loaderClass.Create();
    Ptr<ResourceLoader> loader((ResourceLoader*)obj);
    loader->uniqueId = UniquePoolCounter++;
    loader->Setup();
    this->loaders.Append(loader);
    this->extensionMap.Add(ext, this->loaders.Size() - 1);
    this->typeMap.Add(&loaderClass, this->loaders.Size() - 1);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceServer::DeregisterStreamLoader(const Util::StringAtom& ext, const Core::Rtti& loaderClass)
{
    n_assert(this->open);
    n_assert(loaderClass.IsDerivedFrom(ResourceLoader::RTTI));
    n_assert(this->extensionMap.Contains(ext));
    n_assert(this->typeMap.Contains(&loaderClass));

    IndexT loaderIdx = this->extensionMap[ext];
    n_assert(this->typeMap[&loaderClass] == loaderIdx);
    Ptr<ResourceLoader> loader = this->loaders[loaderIdx];
    this->loaders[loaderIdx] = nullptr;
    this->extensionMap.Erase(ext);
    this->typeMap.Erase(&loaderClass);
    
    loader->ClearPendingUnloads();
    for (auto& kvp : loader->ids)
    {
        const auto resource = loader->resources[kvp.Value()];
        if (loader->GetUsage(kvp.Value()) != 0)
        {
            const Resources::ResourceName& name = kvp.Key();
            Util::String msg = Util::String::Sprintf("Resource <%s> (id %d) from pool %d is not unloaded, usage is '%d'\n", name.Value(), resource.resourceId, loader->uniqueId, loader->GetUsage(kvp.Value()));
            Core::SysFunc::DebugOut(msg.AsCharPtr());
        }
    }
    loader = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceServer::LoadDefaultResources()
{
    n_assert(this->open);
    IndexT i;
    for (i = 0; i < this->loaders.Size(); i++)
    {
        this->loaders[i]->LoadFallbackResources();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceServer::Update(IndexT frameIndex)
{
    N_SCOPE(Update, Resources);
    IndexT i;
    for (i = 0; i < this->loaders.Size(); i++)
    {
        const Ptr<ResourceLoader>& loader = this->loaders[i];
        loader->Update(frameIndex);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceServer::DiscardResources(const Util::StringAtom& tag)
{
    IndexT i;
    for (i = 0; i < this->loaders.Size(); i++)
    {
        const Ptr<ResourceLoader>& loader = this->loaders[i];
        loader->DiscardByTag(tag);
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
ResourceServer::HasPendingResources()
{
    IndexT i;
    for (i = 0; i < this->loaders.Size(); i++)
    {
        const Ptr<ResourceLoader>& loader = this->loaders[i];
        if (loader->IsA(ResourceLoader::RTTI))
        {
            const Ptr<ResourceLoader>& pool = loader.cast<ResourceLoader>();
            if (!pool->pendingLoads.IsEmpty())
                return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
Core::Rtti*
ResourceServer::GetType(const Resources::ResourceId id)
{
    // get id of loader
    const Ids::Id8 loaderid = id.loaderIndex;

    // get resource loader by extension
    n_assert(this->loaders.Size() > loaderid);
    const Ptr<ResourceLoader>& loader = this->loaders[loaderid];
    return loader->GetRtti();
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceServer::WaitForLoaderThread()
{
    for (const Ptr<ResourceLoader>& loader : this->loaders)
    {
        loader->streamerThread->Wait();
    }
}

} // namespace Resources
