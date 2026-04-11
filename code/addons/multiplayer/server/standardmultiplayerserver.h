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
    void OnMessageReceived(ClientConnection* connection, Timing::Time recvTime, byte* data, size_t size) override;
    void OnTick() override;
    void OnFrame() override;
};
    
} // namespace Multiplayer