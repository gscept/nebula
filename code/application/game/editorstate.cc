//------------------------------------------------------------------------------
//  @file editorstate.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editorstate.h"
namespace Game
{

__ImplementSingleton(Game::EditorState);

//------------------------------------------------------------------------------
/**
*/
EditorState::EditorState()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
EditorState::~EditorState()
{
    __DestructSingleton;
}

} // namespace Game
