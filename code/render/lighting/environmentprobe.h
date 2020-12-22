#pragma once
//------------------------------------------------------------------------------
/**
    @class Lighting::EnvironmentProbe
    
    An environment probe contains information about the environment and irradiance map being used in an area.

    (C) 2012-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/managedtexture.h"
namespace Lighting
{
class EnvironmentProbe : public Core::RefCounted
{
    __DeclareClass(EnvironmentProbe);
public:
    /// constructor
    EnvironmentProbe();
    /// destructor
    virtual ~EnvironmentProbe();

    /// assign reflection map resource, returns true if a texture was loaded
    bool AssignReflectionMap(const Resources::ResourceId& refl);
    /// assign reflection map using texture
    void AssignReflectionMap(const Ptr<Resources::ManagedTexture>& refl);
    /// assign irradiance map resource, returns true if a texture was loaded
    bool AssignIrradianceMap(const Resources::ResourceId& irr);
    /// assign irradiance map using texture
    void AssignIrradianceMap(const Ptr<Resources::ManagedTexture>& irr);    
    /// assign depth map resource, returns true if a texture was loaded
    bool AssignDepthMap(const Resources::ResourceId& irr);
    /// assign depth map using texture
    void AssignDepthMap(const Ptr<Resources::ManagedTexture>& irr);

    /// discard probe, unloads textures
    void Discard();

    /// get reflection map
    const Ptr<Resources::ManagedTexture>& GetReflectionMap() const;
    /// get irradiance map
    const Ptr<Resources::ManagedTexture>& GetIrradianceMap() const;
    /// get depth map
    const Ptr<Resources::ManagedTexture>& GetDepthMap() const;

    /// the default environment probe which is automatically assigned to all model entitys upon startup
    static Ptr<EnvironmentProbe> DefaultEnvironmentProbe;

private:
    
    Ptr<Resources::ManagedTexture> reflectionMap;
    Ptr<Resources::ManagedTexture> irradianceMap;
    Ptr<Resources::ManagedTexture> depthMap;
};

//------------------------------------------------------------------------------
/**
*/
inline void
EnvironmentProbe::AssignReflectionMap(const Ptr<Resources::ManagedTexture>& refl)
{
    n_assert(refl.isvalid());
    this->reflectionMap = refl;
}

//------------------------------------------------------------------------------
/**
*/
inline void
EnvironmentProbe::AssignIrradianceMap(const Ptr<Resources::ManagedTexture>& irr)
{
    n_assert(irr.isvalid());
    this->irradianceMap = irr;
}

//------------------------------------------------------------------------------
/**
*/
inline void
EnvironmentProbe::AssignDepthMap(const Ptr<Resources::ManagedTexture>& depth)
{
    n_assert(depth.isvalid());
    this->depthMap = depth;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Resources::ManagedTexture>&
EnvironmentProbe::GetReflectionMap() const
{
    return this->reflectionMap;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Resources::ManagedTexture>&
EnvironmentProbe::GetIrradianceMap() const
{
    return this->irradianceMap;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Resources::ManagedTexture>&
EnvironmentProbe::GetDepthMap() const
{
    return this->depthMap;
}

} // namespace Lighting