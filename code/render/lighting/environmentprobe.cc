//------------------------------------------------------------------------------
//  environmentprobe.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "environmentprobe.h"
#include "resources/resourcemanager.h"

using namespace Resources;
using namespace CoreGraphics;
namespace Lighting
{
__ImplementClass(Lighting::EnvironmentProbe, 'ENPR', Core::RefCounted);

Ptr<EnvironmentProbe> EnvironmentProbe::DefaultEnvironmentProbe = NULL;

//------------------------------------------------------------------------------
/**
*/
EnvironmentProbe::EnvironmentProbe()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
EnvironmentProbe::~EnvironmentProbe()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
EnvironmentProbe::Discard()
{
    // get resource manager
    Ptr<ResourceManager> resManager = ResourceManager::Instance();
    resManager->DiscardManagedResource(this->reflectionMap.upcast<ManagedResource>());
    resManager->DiscardManagedResource(this->irradianceMap.upcast<ManagedResource>());
    this->reflectionMap = 0;
    this->irradianceMap = 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
EnvironmentProbe::AssignReflectionMap(const Resources::ResourceId& refl)
{
    Ptr<ResourceManager> resManager = ResourceManager::Instance();

    // avoid potentially reloading the resource if its the current one
    if (refl.IsValid())
    {
        if (this->reflectionMap.isvalid())
        {
            if (this->reflectionMap->GetResourceId() != refl)
            {
                resManager->DiscardManagedResource(this->reflectionMap.upcast<ManagedResource>());
                this->reflectionMap = resManager->CreateManagedResource(Texture::RTTI, refl).downcast<ManagedTexture>();
            }
            else
            {
                return false;
            }
        }
        else
        {
            this->reflectionMap = resManager->CreateManagedResource(Texture::RTTI, refl).downcast<ManagedTexture>();
        }
        return true;
    }   
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
EnvironmentProbe::AssignIrradianceMap(const Resources::ResourceId& irr)
{
    Ptr<ResourceManager> resManager = ResourceManager::Instance();

    // only apply if resource is valid, 
    if (irr.IsValid())
    {
        if (this->irradianceMap.isvalid())
        {
            if (this->irradianceMap->GetResourceId() != irr)
            {
                resManager->DiscardManagedResource(this->irradianceMap.upcast<ManagedResource>());
                this->irradianceMap = resManager->CreateManagedResource(Texture::RTTI, irr).downcast<ManagedTexture>();
            }
            else
            {
                return false;
            }
        }
        else
        {
            this->irradianceMap = resManager->CreateManagedResource(Texture::RTTI, irr).downcast<ManagedTexture>();
        }
        return true;
    }
    return false;
}


//------------------------------------------------------------------------------
/**
*/
bool
EnvironmentProbe::AssignDepthMap(const Resources::ResourceId& depth)
{
    Ptr<ResourceManager> resManager = ResourceManager::Instance();

    // only apply if resource is valid, 
    if (depth.IsValid())
    {
        if (this->depthMap.isvalid())
        {
            if (this->depthMap->GetResourceId() != depth)
            {
                resManager->DiscardManagedResource(this->depthMap.upcast<ManagedResource>());
                this->depthMap = resManager->CreateManagedResource(Texture::RTTI, depth).downcast<ManagedTexture>();
            }
            else
            {
                return false;
            }
        }
        else
        {
            this->depthMap = resManager->CreateManagedResource(Texture::RTTI, depth).downcast<ManagedTexture>();
        }
        return true;
    }
    return false;
}

} // namespace Lighting