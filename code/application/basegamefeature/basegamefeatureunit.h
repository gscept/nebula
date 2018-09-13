#pragma once
//------------------------------------------------------------------------------
/**
    @class Game::BaseGameFeatureUnit
    
    The BaseGameFeatureUnit creates everything to allow load and run a game level.
    Therefore it creates managers to allow creation and handling of
    entities and components. 

    If you wanna use your own managers (other specilized entitymanager), derive from this class 
    and overwrite OnActivate() OnDeactivate().

    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "game/featureunit.h"
#include "math/bbox.h"
#include "managers/entitymanager.h"
#include "managers/componentmanager.h"
#include "basegamefeature/components/transformcomponent.h"

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

    /// called from BaseGameFeatureUnit::ActivateProperties()
    virtual void OnActivate();
    /// called from BaseGameFeatureUnit::DeactivateProperties()
    virtual void OnDeactivate();
         
    /// called at the end of the feature trigger cycle
    virtual void OnEndFrame();
    /// called when game debug visualization is on
    virtual void OnRenderDebug();
    /// 
    virtual void OnFrame();

    /// set optional world dimensions
    void SetWorldDimensions(const Math::bbox& box);
    /// get world dimensions
    const Math::bbox& GetWorldDimensions() const;
    /// setup an empty game world
    virtual void SetupEmptyWorld();
    /// cleanup the game world
    virtual void CleanupWorld();

protected:
    
    Ptr<Game::EntityManager> entityManager;
	Ptr<Game::TransformComponent> transformComponent;
	Ptr<Game::ComponentManager> componentManager;

    Math::bbox worldBox;
};

//------------------------------------------------------------------------------
/**
*/
inline void
BaseGameFeatureUnit::SetWorldDimensions(const Math::bbox& box)
{
    this->worldBox = box;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::bbox&
BaseGameFeatureUnit::GetWorldDimensions() const
{
    return this->worldBox;
}

} // namespace BaseGameFeature
//------------------------------------------------------------------------------
