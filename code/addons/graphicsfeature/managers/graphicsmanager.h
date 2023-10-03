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

private:
    /// constructor
    GraphicsManager();
    /// destructor
    ~GraphicsManager();

    void InitCreateModelProcessor();
    void InitUpdateModelTransformProcessor();
    static void OnDecay();

    static void OnCleanup(Game::World* world);
};

} // namespace GraphicsFeature
