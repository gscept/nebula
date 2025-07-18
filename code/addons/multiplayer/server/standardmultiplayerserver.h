#pragma once
//------------------------------------------------------------------------------
/**
    @class  Multiplayer::StandardMultiplayerServer

    @copyright
    (C) 2025 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "multiplayer/server/basemultiplayerserver.h"
namespace Multiplayer
{

class StandardMultiplayerServer : public BaseMultiplayerServer
{
public:
    StandardMultiplayerServer() {};
    ~StandardMultiplayerServer() {};
    bool Open() override;
    void Close() override;

    bool OnClientIsConnecting(ClientConnection* connection) override;
    void OnClientConnected(ClientConnection* connection) override;
    void OnClientDisconnected(ClientConnection* connection) override;
    void OnMessageReceived(Timing::Time recvTime, uint32_t connectionId, byte* data, size_t size) override;
};
    
} // namespace Multiplayer