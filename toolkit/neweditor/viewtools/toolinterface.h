#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::ToolInterface

    What every viewport tool is: it reads one frame of canvas input, which includes what the
    transform handle widget did. The tools replace the ones the old editor kept in the scene window,
    and they keep the same shape, but they read the ToolView the viewport panel fills rather than
    the input server, because ImGui owns the mouse and the keyboard while the editor draws.

    A tool is not ref counted. The ToolContext owns the instances, one of each, and hands over the
    scene through itself, because the scene changes with the active tab.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "neweditor/viewtools/toolview.h"
#include "game/entity.h"
#include "math/quat.h"
#include "math/vec3.h"

namespace Editor
{

class ToolContext;

//------------------------------------------------------------------------------
/**
    One entity's complete transform: the three components the transform tools write. A tool drags
    these rather than component pointers, so the drag reads the authored state once and a preview
    never has to worry about what it is looking at.
*/
struct TransformEntry
{
    Game::Entity entity = Game::Entity::Invalid();
    Math::vec3 position = Math::vec3(0);
    Math::quat orientation = Math::quat(0, 0, 0, 1);
    Math::vec3 scale = Math::vec3(1);
};

class ToolInterface
{
public:
    virtual ~ToolInterface() = default;

    /// read this frame's canvas input, which includes what the handle widget did
    virtual void Update(const ToolView& view) = 0;

    /// true while the tool is changing something, which is what makes escape abort it rather than
    /// switch
    virtual bool IsModifying() const = 0;
    /// put back whatever an in-progress change has written
    virtual void Abort() {}

    /// the name the tool command and the status bar show
    virtual const char* Name() const = 0;

    /// the context the tool runs against: the scene, the selection and the transform plumbing
    void SetContext(ToolContext* context) { this->context = context; }

protected:
    ToolContext* context = nullptr;
};

} // namespace Editor
