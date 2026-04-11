#pragma once
//------------------------------------------------------------------------------
/**
    @file  serverprocessors.h

    @copyright
    (C) 2025 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
namespace Multiplayer
{

class BaseMultiplayerServer;

void SetupServerProcessors(BaseMultiplayerServer* server);
void ShutdownServerProcessors();
void SetServerProcessorsActive(bool active);

} // namespace Multiplayer