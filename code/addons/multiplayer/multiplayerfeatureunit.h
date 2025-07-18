#pragma once
//------------------------------------------------------------------------------
/**
    @class Multiplayer::MultiplayerFeatureUnit

    @copyright
    (C) 2025 Individual contributors, see AUTHORS file
*/
#include "game/featureunit.h"
#include "server/basemultiplayerserver.h"
#include "client/basemultiplayerclient.h"

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
    
    void OnAttach() override;
    void OnActivate() override;
    void OnDeactivate() override;
    void OnBeginFrame() override;
    void OnFrame() override;
    void OnEndFrame() override;
    
    virtual void OnRenderDebug();
    
    /// returns true if this instance is the server.
    bool IsServer() const;
    /// Set the server that should be used (if any). You must set this before activating the feature unit.
    void SetServer(BaseMultiplayerServer* server);
    /// Set the client that should be used (if any). You must set this before activating the feature unit.
    void SetClient(BaseMultiplayerClient* client);

private:
    BaseMultiplayerServer* server = nullptr;
    BaseMultiplayerClient* client = nullptr;

};

//--------------------------------------------------------------------------
/**
*/
inline bool
MultiplayerFeatureUnit::IsServer() const
{
    return this->server != nullptr;
}

//--------------------------------------------------------------------------
/**
*/
inline void
MultiplayerFeatureUnit::SetServer(BaseMultiplayerServer* server)
{
    this->server = server;
}

//--------------------------------------------------------------------------
/**
*/
inline void
MultiplayerFeatureUnit::SetClient(BaseMultiplayerClient* client)
{
    this->client = client;
}

} // namespace Scripting
//------------------------------------------------------------------------------
