#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::ViewportPanel

    The scene view. The panel draws the game through an offscreen target sized to its canvas and
    shows the result as an image. That is what makes the viewport a panel instead of the window.

    The panel is the editor's half. A render side derives from this and answers the target, the draw
    and the mouse. A locked cursor reports nothing through ImGui, so the mouse is the render side's.

    The camera gestures are read here, because the panel owns the keyboard and the mouse while it
    has focus. The keys move the camera, the wheel changes the distance to the point it orbits
    around, and the button that holds the cursor for a drag turns, swings or slides the view.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/ptr.h"
#include "game/entity.h"
#include "imgui.h"
#include "math/mat4.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "neweditor/commands/instantiateprefabcommand.h"
#include "neweditor/core/editorpanel.h"
#include "neweditor/viewtools/editorcamera.h"
#include "util/array.h"
#include "util/string.h"

namespace Editor
{

class ViewportPanel : public EditorPanel
{
    __DeclareAbstractClass(ViewportPanel)
public:
    ViewportPanel();
    virtual ~ViewportPanel();

    /// size the target, draw the canvas and read the input
    void Draw() override;

    /// the panel gained or lost focus, which decides whether it drives the camera
    void OnFocus(bool focused) override;

    bool BeginWindow() override;
    void EndWindow() override;

    /// the size the panel starts with, before its canvas has ever been laid out. A render side needs
    /// a viewport before the first draw
    static const int InitialTargetWidth = 640;
    static const int InitialTargetHeight = 360;

    /// the size of the target in physical pixels. The status bar shows it
    int TargetWidth() const;
    int TargetHeight() const;

    /// the field of view in degrees. A render side needs it to build its own projection
    float FieldOfView() const;

protected:
    bool OnSetup() override;

    // -- What a render side answers --

    /// the offscreen target for a canvas size. Made the first time, resized after. The render side
    /// makes its own context current inside, because the panel cannot
    virtual ImTextureID OnTarget(int width, int height) = 0;

    /// the scene drawn with this camera into the target. The render side draws it later in the
    /// frame, after the panels have drawn, which is when its draw data is submitted
    virtual void OnRender(const Math::mat4& view, const Math::mat4& projection) = 0;

    /// the mouse. A locked cursor reports no position and no delta through ImGui, so these are the
    /// render side's
    virtual void OnLockMouse(bool locked) = 0;
    virtual void OnWarpMouse(const Math::vec2& pos) = 0;
    virtual Math::vec2 OnMouseDelta() const = 0;

    /// the canvas was not drawn this frame. A target that is not shown would still be drawn into,
    /// so the render side hands its target back here
    virtual void OnCanvasClosed() {}

private:
    enum ViewMode
    {
        ViewScene = 0,
        ViewGame = 1,
        ViewLighting = 2,
        ViewRendering = 3,
    };

    float fieldOfView;
    float moveSpeed;
    int viewMode;

    /// the camera drag, as the button that owns the cursor. The right button turns in place, the
    /// middle button swings the camera around the pivot, and the middle button with shift slides
    /// the view. -1 when no drag holds the cursor, and the cursor is hidden and locked for as long
    /// as one does
    int cameraButton;
    /// the point the cursor is re-centered at while a camera drag holds it, which is the center of
    /// the canvas
    Math::vec2 dragAnchor;
    Editor::EditorCameraInput pendingInput;

    /// true while a left button gesture that began over the canvas is still held. This is the
    /// panel's stand-in for the old viewport's mouse capture: a gesture that began in another panel
    /// is never seen by the tools
    bool mouseDownOnCanvas;

    bool focused;

    /// A file dragged in from the file browser. Invalid means no drop is in progress.
    Ptr<Editor::InstantiatePrefabCommand> fileDrop;
    /// the cursor on the y=0 plane, as of the last frame the drag was over the canvas
    Math::vec3 fileDropPoint;

    /// the size of the target the render side has now, in physical pixels
    int targetWidth;
    int targetHeight;

    /// true if the panel drives the camera this frame
    bool CanNavigate() const;
    /// the tracked input for this frame, which is consumed by the camera
    Editor::EditorCameraInput GatherInput();
    /// re-center the cursor at the anchor, which is what keeps the move events coming while it is
    /// locked
    void CenterMouse();

    /// put the dragged file's entities into the scene and open the macro the drop runs in
    void BeginFileDrop(const char* path, const Math::vec3& onPlane);
    /// move the entities to the cursor as previews
    void PreviewFileDrop(const Math::vec3& onPlane);
    /// the release over the canvas: write the position for real and close the macro
    void CommitFileDrop();
    /// the drag ended without a release over the canvas, or the panel closed before it could
    /// commit: close the macro and undo it
    void AbortFileDrop();
    /// the roots of the instantiation, the entities that move under the cursor, read from the
    /// command. The children of the instantiation follow those through the parent chain
    Util::Array<Game::Entity> FileDropRoots() const;

    void SetViewMode(int mode);
    const char* ViewModeName(int mode) const;

    /// commands
    void CommandView(const Util::Array<Util::String>& args);
    void CommandFov(const Util::Array<Util::String>& args);
    void CommandSpeed(const Util::Array<Util::String>& args);
    void CommandFocus(const Util::Array<Util::String>& args);
    void CommandReset(const Util::Array<Util::String>& args);
};

} // namespace Editor
//------------------------------------------------------------------------------
