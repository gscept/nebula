#pragma once
//------------------------------------------------------------------------------
/**
    @class  PhysicsFeature::CharacterManager

    @copyright
    (C) 2025 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/manager.h"
#include "game/world.h"

namespace PhysicsFeature
{

class CharacterManager : public Game::Manager
{
    __DeclareClass(CharacterManager)
    __DeclareSingleton(CharacterManager)
public:
    CharacterManager();
    virtual ~CharacterManager();

    void OnActivate() override;
    void OnDeactivate() override;
    void OnDecay() override;
    void OnCleanup(Game::World* world) override;
};

} // namespace PhysicsFeature
