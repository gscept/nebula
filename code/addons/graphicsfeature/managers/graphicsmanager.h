#pragma once
//------------------------------------------------------------------------------
/**
    @class  GraphicsFeature::GraphicsManager

    Handles logic for connecting the game layer with the render layer.
    Also handles simple graphics entities such as static models and similar.

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/manager.h"
#include "game/category.h"
#include "graphics/graphicsentity.h"
#include "game/componentinspection.h"
#include "components/camera.h"
#include "components/decal.h"
#include "components/lighting.h"
#include "components/model.h"
#include "components/gi.h"
#include "components/terrain.h"

namespace GraphicsFeature
{

class GraphicsManager : public Game::Manager
{
    __DeclareClass(GraphicsManager)
public:
    
    GraphicsManager();
    virtual ~GraphicsManager();

    void OnActivate() override;
    void OnDeactivate() override;
    void OnDecay() override;
    void OnCleanup(Game::World* world) override;

    /// called automatically when a model needs to be initialized
    static void InitModel(Game::World*, Game::Entity, Model*);
    /// called automatically when a point light needs to be initialized
    static void InitPointLight(Game::World* world, Game::Entity entity, PointLight* light);
    /// called automatically when a spot light needs to be initialized
    static void InitSpotLight(Game::World* world, Game::Entity entity, SpotLight* light);
    /// called automatically when an area light needs to be initialized
    static void InitAreaLight(Game::World* world, Game::Entity entity, AreaLight* light);
    /// called automatically when a decal needs to be initialized
    static void InitDecal(Game::World* world, Game::Entity entity, Decal* decal);
    /// called automatically when a ddgi volume needs to be initialized
    static void InitDDGIVolume(Game::World* world, Game::Entity entity, DDGIVolume* volume);
    /// Called automatically when a terrain needs to be initialized
    static void InitTerrain(Game::World* world, Game::Entity entity, Terrain* terrain);

private:
    void InitUpdateModelTransformProcessor();
    void InitUpdateLightTransformProcessor();
    void InitUpdateDecalTransformProcessor();
    void InitUpdateDDGIVolumeTransformProcessor();
};

} // namespace GraphicsFeature

namespace Game
{
template <>
void ComponentDrawFuncT<GraphicsFeature::AreaLightShape>(Game::Entity, ComponentId, void*, bool*);
}
