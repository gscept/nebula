#pragma once
//------------------------------------------------------------------------------
/**
    Resolves local hierarchical transforms into the mandatory world transform
    components.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/manager.h"

namespace Game
{
class World;

class HierarchyManager : public Game::Manager
{
    __DeclareClass(HierarchyManager)
public:
    /// Resolve all hierarchical transforms in a world.
    static void Resolve(World* world);

    void OnActivate() override;
    void OnDeactivate() override;
    void OnBeginFrame() override;
    void OnEndFrame() override;
};

} // namespace Game
