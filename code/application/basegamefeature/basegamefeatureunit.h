#pragma once
//------------------------------------------------------------------------------
/**
    @class BaseGameFeature::BaseGameFeatureUnit
    
    The BaseGameFeatureUnit creates everything to allow load and run a game level.
    Therefore it creates managers to allow creation and handling of
    entities, properties and attributes. 

    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "game/featureunit.h"
#include "math/bbox.h"

//------------------------------------------------------------------------------
namespace BaseGameFeature
{

class BaseGameFeatureUnit : public Game::FeatureUnit    
{
    __DeclareClass(BaseGameFeatureUnit)
    __DeclareSingleton(BaseGameFeatureUnit)

public:

    /// constructor
    BaseGameFeatureUnit();
    /// destructor
    virtual ~BaseGameFeatureUnit();

    /// Called upon activation of feature unit
    void OnActivate() override;
    /// Called upon deactivation of feature unit
    void OnDeactivate() override;
         
    /// called at the end of the feature trigger cycle
    void OnEndFrame() override;
    /// called when game debug visualization is on
    void OnRenderDebug() override;
    /// called at the beginning of a frame
    void OnFrame() override;
    
protected:
    Game::ManagerHandle blueprintManager;
    Game::ManagerHandle timeManager;
};

} // namespace BaseGameFeature
//------------------------------------------------------------------------------
