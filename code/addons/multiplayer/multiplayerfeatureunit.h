#pragma once
//------------------------------------------------------------------------------
/**
    @class Multiplayer::MultiplayerFeatureUnit

    @copyright
    (C) 2025 Individual contributors, see AUTHORS file
*/
#include "game/featureunit.h"
#include "server/multiplayerserver.h"
#include "client/multiplayerclient.h"

//------------------------------------------------------------------------------
namespace Multiplayer
{

class MultiplayerFeatureUnit : public Game::FeatureUnit
{
    __DeclareClass(MultiplayerFeatureUnit)
    __DeclareSingleton(MultiplayerFeatureUnit)

public:

    /// constructor
    MultiplayerFeatureUnit();
    /// destructor
    ~MultiplayerFeatureUnit();
    
    void OnActivate() override;
    void OnDeactivate() override;
    void OnBeginFrame() override;
    void OnFrame() override;
    void OnEndFrame() override;
    
    virtual void OnRenderDebug();

    float temp_f = 0;

private:
    Ptr<MultiplayerServer> server = nullptr;
    Ptr<MultiplayerClient> client = nullptr;
    
};

} // namespace Scripting
//------------------------------------------------------------------------------
