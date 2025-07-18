#pragma once
//------------------------------------------------------------------------------
/**
    @class  Multiplayer::StandardMultiplayerClient

    @copyright
    (C) 2025 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "multiplayer/client/basemultiplayerclient.h"
#include "game/entity.h"


namespace Multiplayer
{

class StandardMultiplayerClient : public BaseMultiplayerClient
{
public:
    StandardMultiplayerClient() {};
    ~StandardMultiplayerClient() {};
    bool Open() override;
    void Close() override;

    void OnIsConnecting() override;
    void OnConnected() override;
    void OnDisconnected() override;
    void OnMessageReceived(SteamNetworkingMessage_t* msg) override;

private:
    Util::HashTable<uint, Game::Entity> networkEntities;
};
    
} // namespace Multiplayer