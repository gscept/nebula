//------------------------------------------------------------------------------
//  modelserver.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "models/modelserver.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "resources/resourcemanager.h"
#include "resources/simpleresourcemapper.h"
#include "models/streammodelloader.h"
#include "models/visresolver.h"

namespace Models
{
__ImplementClass(Models::ModelServer, 'MDLS', Core::RefCounted);
__ImplementSingleton(Models::ModelServer);

using namespace Core;
using namespace Util;
using namespace IO;
using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
ModelServer::ModelServer() :
    isOpen(false),
    curModelNodeInstanceIndex(InvalidIndex),
    maxModelNodeInstanceIndex(255)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ModelServer::~ModelServer()
{
    if (this->IsOpen())
    {
        this->Close();
    }
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
ModelServer::Open()
{
    n_assert(!this->IsOpen())
    this->isOpen = true;

    // create the visibility resolver singleton
    this->visResolver = VisResolver::Create();
    this->visResolver->Open();

    // setup a SimpleResourceMapper if no external mapper is set
    if (!this->modelResourceMapper.isvalid())
    {
        Ptr<SimpleResourceMapper> resMapper = SimpleResourceMapper::Create();
        resMapper->SetPlaceholderResourceId(ResourceId("mdl:system/placeholder.n3"));
        resMapper->SetAsyncEnabled(true);
        resMapper->SetResourceClass(Model::RTTI);
        resMapper->SetResourceLoaderClass(StreamModelLoader::RTTI);
        resMapper->SetManagedResourceClass(ManagedModel::RTTI);        
		ResourceManager::Instance()->AttachMapper(resMapper.upcast<ResourceMapper>());
    }
    else
    {
        n_assert(this->modelResourceMapper->GetResourceType() == Model::RTTI);
        ResourceManager::Instance()->AttachMapper(this->modelResourceMapper);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ModelServer::Close()
{
    n_assert(this->IsOpen());
 
    // detach our model resource mapper
    ResourceManager::Instance()->RemoveMapper(Model::RTTI);
    this->isOpen = false;

    // release the visibility resolver singleton
    this->visResolver->Close();
    this->visResolver = 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
ModelServer::HasManagedModel(const ResourceId& resId) const
{
    return ResourceManager::Instance()->HasManagedResource(resId);
}

//------------------------------------------------------------------------------
/**
*/
Ptr<ManagedModel>
ModelServer::LoadManagedModel(const ResourceId& resId, bool sync)
{
    return ResourceManager::Instance()->CreateManagedResource(Model::RTTI, resId, 0, sync).downcast<ManagedModel>();
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<ManagedModel>&
ModelServer::LookupManagedModel(const ResourceId& resId) const
{
    n_assert(this->HasManagedModel(resId));
    return ResourceManager::Instance()->LookupManagedResource(resId).downcast<ManagedModel>();
}

//------------------------------------------------------------------------------
/**
*/
void
ModelServer::DiscardManagedModel(const Ptr<ManagedModel>& managedModel) const
{
    ResourceManager::Instance()->DiscardManagedResource(managedModel.upcast<ManagedResource>());
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
ModelServer::ConsumeNewModelNodeInstanceIndex()
{
    IndexT reservedIndex = this->curModelNodeInstanceIndex;        
    this->curModelNodeInstanceIndex++;
    if (this->curModelNodeInstanceIndex > this->maxModelNodeInstanceIndex)
    {
        this->curModelNodeInstanceIndex = 1;
    }
    return reservedIndex;
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelServer::ResetModelNodeInstanceIndex()
{
    this->curModelNodeInstanceIndex = 1;
}
} // namespace Models