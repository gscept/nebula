#pragma once
//------------------------------------------------------------------------------
/**
    Editor::UIManager

    (C) 2019-2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/manager.h"
#include "coregraphics/texture.h"

namespace Editor
{

class UIManager : public Game::Manager
{
    __DeclareClass(UIManager)
public:
    UIManager();
    ~UIManager();

    void OnActivate() override;
    void OnDeactivate() override;
    void OnBeginFrame() override;
};

namespace UI
{
namespace Icons
{

typedef uint64_t texturehandle_t;

extern texturehandle_t play;
extern texturehandle_t pause;
extern texturehandle_t stop;
extern texturehandle_t game;
extern texturehandle_t environment;
extern texturehandle_t light;

}
}

} // namespace Editor
