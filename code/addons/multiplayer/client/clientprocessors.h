#pragma once
//------------------------------------------------------------------------------
/**
    @file  clientprocessors.h

    @copyright
    (C) 2025 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
namespace Multiplayer
{

class BaseMultiplayerClient;

void SetupClientProcessors(BaseMultiplayerClient* client);
void ShutdownClientProcessors();

} // namespace Multiplayer