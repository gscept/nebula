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
    void OnFrame() override;
    static const Util::String GetEditorUIIniPath();
private:
    /// fugly
    bool delayedImguiLoad = true;
    static const char* editorUIPath;
};

} // namespace Editor
