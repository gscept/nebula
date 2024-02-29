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
#include "graphicsfeature/components/graphicsfeature.h"

namespace GraphicsFeature
{

class GraphicsManager
{
    __DeclareSingleton(GraphicsManager);
public:
    /// retrieve the api
    static Game::ManagerAPI Create();

    /// destroy entity manager
    static void Destroy();

    /// called automatically when a model needs to be initialized
    static void InitModel(Game::World*, Game::Entity, Model*);
    /// called automatically when a model needs to be initialized
    static void InitPointLight(Game::World* world, Game::Entity entity, PointLight* light);

private:
    /// constructor
    GraphicsManager();
    /// destructor
    ~GraphicsManager();


    void InitCreatePointLightProcessor();
    void InitUpdateModelTransformProcessor();
    static void OnDecay();

    static void OnCleanup(Game::World* world);
};

} // namespace GraphicsFeature
