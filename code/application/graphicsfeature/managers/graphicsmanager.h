#pragma once
//------------------------------------------------------------------------------
/**
    @class  GraphicsFeature::GraphicsManager

    Handles logic for connecting the game layer with the render layer.
    Also handles simple graphics entities such as static models and similar.

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/manager.h"
#include "graphicsfeature/properties/graphics.h"
#include "game/category.h"

namespace GraphicsFeature
{

struct ModelEntityData
{
    Graphics::GraphicsEntityId gid;
};

class GraphicsManager
{
    __DeclareSingleton(GraphicsManager);
public:
    /// retrieve the api
    static Game::ManagerAPI Create();

    /// destroy entity manager
    static void Destroy();

    struct Pids
    {
        Game::PropertyId modelEntityData;
    } pids;
private:
    /// constructor
    GraphicsManager();
    /// destructor
    ~GraphicsManager();

    void InitCreateModelProcessor();
    void InitDestroyModelProcessor();
    void InitUpdateModelTransformProcessor();

    static void OnBeginFrame();
};

} // namespace GraphicsFeature
