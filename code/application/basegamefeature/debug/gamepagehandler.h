#pragma once
#ifndef DEBUG_GAMEPAGEHANDLER_H
#define DEBUG_GAMEPAGEHANDLER_H
//------------------------------------------------------------------------------
/**
    @class Debug::GamePageHandler
    
    Displays info about currently running Nebula BaseGameFeatureUnit system.
    
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
#include "http/httprequesthandler.h"

//------------------------------------------------------------------------------
namespace Debug
{
class GamePageHandler : public Http::HttpRequestHandler
{
    __DeclareClass(GamePageHandler);
public:
    /// constructor
	GamePageHandler();
    /// handle a http request, the handler is expected to fill the content stream with response data
    void HandleRequest(const Ptr<Http::HttpRequest>& request);
	void InspectComponent(const Util::FourCC& fourcc, const Ptr<Http::HttpRequest>& request);
};

} // namespace Debug
//------------------------------------------------------------------------------
#endif
