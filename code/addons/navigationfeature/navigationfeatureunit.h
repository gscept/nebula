#pragma once
//------------------------------------------------------------------------------
/**
    @class NavigationFeature::NavigationFeatureUnit

        
    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
#include "game/featureunit.h"
#include "graphics/graphicsentity.h"
#include "streamnavmeshcache.h"
#include "navbuild/navareas.h"

//------------------------------------------------------------------------------
namespace NavigationFeature
{

class NavigationFeatureUnit : public Game::FeatureUnit
{
    __DeclareClass(NavigationFeatureUnit)
    __DeclareSingleton(NavigationFeatureUnit)

public:

    /// constructor
    NavigationFeatureUnit();
    /// destructor
    ~NavigationFeatureUnit();

    /// register the components
    void OnAttach() override;
    /// Called upon activation of feature unit
    void OnActivate();
    /// Called upon deactivation of feature unit
    void OnDeactivate();

    /// called on begin of frame
    virtual void OnBeginFrame();

    /// called when game debug visualization is on
    virtual void OnRenderDebug();

private:
    Ptr<Navigation::StreamNavMeshCache> navMeshCache;
};

/// render editor ui 
void RenderUI(Graphics::GraphicsEntityId camera);
/// the loader of the baked navmeshes, null before the feature is activated
Navigation::StreamNavMeshCache* GetNavMeshCache();
/// the area table of the project, loaded when the feature is activated
const Navigation::Build::AreaTable& GetAreaTable();
/// load the compiled area table again
void ReloadAreaTable();
/// intersect a ray with the surface of every loaded navmesh, t is in units of dir. Returns false if it misses all
bool RaycastNavMeshes(const Math::point& start, const Math::vector& dir, float& outT);

} // namespace NavigationFeature
//------------------------------------------------------------------------------
