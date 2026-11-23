//------------------------------------------------------------------------------
//  @file neweditor/panels/viewportpanel.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/panels/viewportpanel.h"
#include "neweditor/base/palette.h"
#include "core/cvar.h"
#include "neweditor/commands/setcomponentcommand.h"
#include "neweditor/console/editorcommands.h"
#include "neweditor/core/editorcore.h"
#include "neweditor/imguizmo/imguizmo.h"
#include "neweditor/input/editorkeybinds.h"
#include "neweditor/model/editcommandmanager.h"
#include "neweditor/model/scene.h"
#include "neweditor/model/selectioncontext.h"
#include "neweditor/viewtools/axisindicator.h"
#include "neweditor/viewtools/toolcontext.h"
#include "neweditor/widgets/filebrowser.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/managers/timemanager.h"
#include "game/world.h"
#include "math/bbox.h"
#include "math/line.h"
#include "math/plane.h"

namespace Editor
{

__ImplementAbstractClass(Editor::ViewportPanel, 'VPNL', Editor::EditorPanel);

static const float DefaultFieldOfView = 70.0f;
static const float DefaultMoveSpeed = 6.0f;
static const float ShiftSpeedFactor = 10.0f;
static const float ZoomSpeed = 0.6f;

// The distance a focus stands the camera back from the selection. The base of it is the distance
// that puts the selection's bounding sphere on the edge of the field of view, and the margin is
// what keeps an object from filling the whole canvas. The minimum is for an entity whose bounds are
// a point, which would otherwise have the camera land inside it
static const float FocusMargin = 1.5f;
static const float MinFocusDistance = 1.0f;

// pie will switch to game view if true, and reset to last view when stopped
static Core::CVar* ed_focus_game_on_play = nullptr;

//------------------------------------------------------------------------------
/**
*/
ViewportPanel::ViewportPanel() :
    fieldOfView(DefaultFieldOfView),
    moveSpeed(DefaultMoveSpeed),
    viewMode(ViewScene),
    cameraButton(-1),
    dragAnchor(Math::vec2(0, 0)),
    mouseDownOnCanvas(false),
    focused(false),
    gameCamera(Game::Entity::Invalid()),
    autoViewMode(-1),
    wasPlaying(false),
    fileDropPoint(Math::vec3(0, 0, 0)),
    targetWidth(0),
    targetHeight(0)
{
    this->name = "viewport";
}

//------------------------------------------------------------------------------
/**
*/
ViewportPanel::~ViewportPanel()
{
    // A drop that was in progress when the panel went away has a macro nothing else can close, and
    // the instantiate would stay as an entry the author never committed. The core releases its
    // panels before it discards the scenes, so the command manager is still here to finish the
    // macro off
    if (this->core != nullptr)
        this->AbortFileDrop();
}

//------------------------------------------------------------------------------
/**
    The target is made here rather than at the first draw, because the panel has to have something
    to show before the canvas has ever been laid out.
*/
bool
ViewportPanel::OnSetup()
{
    if (this->OnTarget(InitialTargetWidth, InitialTargetHeight) == 0)
        return false;

    Editor::EditorCommandSystem& commands = this->core->GetCommands();
    commands.Register("view", "view [scene|game|lighting|rendering] - the viewport view mode",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<ViewportPanel, &ViewportPanel::CommandView>(this));
    commands.Register("fov", "fov [degrees] - print or set the viewport field of view",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<ViewportPanel, &ViewportPanel::CommandFov>(this));
    commands.Register("speed", "speed [units per second] - print or set the viewport move speed",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<ViewportPanel, &ViewportPanel::CommandSpeed>(this));
    commands.Register("focus", "focus the camera on the selection",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<ViewportPanel, &ViewportPanel::CommandFocus>(this));
    commands.Register("reset", "reset the viewport camera",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<ViewportPanel, &ViewportPanel::CommandReset>(this));

    this->core->GetKeybinds().Register("view.reset", Input::Key::Home, Input::Modifier::NoModifier, "reset");
    this->core->GetKeybinds().Register("view.focus", Input::Key::F, Input::Modifier::NoModifier, "focus");
    // F2 is the outline's rename key, so the view modes are on the function keys after it
    this->core->GetKeybinds().Register("view.scene", Input::Key::F3, Input::Modifier::NoModifier, "view scene");
    this->core->GetKeybinds().Register("view.game", Input::Key::F4, Input::Modifier::NoModifier, "view game");

    ed_focus_game_on_play = Core::CVarCreate(Core::CVarType::CVar_Int, "ed_focus_game_on_play", "1",
        "Switch the viewport to game view when play starts");

    return true;
}

//------------------------------------------------------------------------------
/**
    A closed panel never draws, so this is where the render side is told the canvas is not being
    used. Otherwise it keeps drawing into a target that is not shown.
*/
bool
ViewportPanel::BeginWindow()
{
    // The transition is read here rather than in Draw, because a collapsed panel does not draw but its
    // mode still has to follow play
    const bool moved = this->OnPlayTransition();

    if (!this->open && this->fileDrop.isvalid())
        this->AbortFileDrop();

    // The core only calls EndWindow for an open panel, so the padding is pushed for an open one only.
    // Pushed for a closed panel it is never popped and leaks into the next window
    if (this->open)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    }
    if (!EditorPanel::BeginWindow())
    {
        // A closed panel does not draw, so a drag that was held when it closed has no frame to
        // read its release on and keeps the cursor locked. It lets go here instead
        if (this->cameraButton >= 0)
        {
            this->cameraButton = -1;
            this->core->SetCameraCapture(false);
            this->OnLockMouse(false);
        }

        this->OnCanvasClosed();
        return false;
    }

    // The frame the panel moves to game view is also the frame it takes the keyboard on, because the
    // keys the panel no longer reads are the game's. Without the focus they stay with whichever panel
    // has it
    if (moved)
        ImGui::SetWindowFocus();

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
ViewportPanel::EndWindow()
{
    EditorPanel::EndWindow();
    ImGui::PopStyleVar();
}

//------------------------------------------------------------------------------
/**
*/
void
ViewportPanel::Draw()
{
    // The view belongs to the scene, so this is the camera of whichever tab is showing
    Editor::EditorCamera& cam = this->core->GetScene()->GetCamera();

    ImGuiStyle& style = ImGui::GetStyle();

    // The window has no padding because of the canvas, so the header and the overlay line get their
    // own margin
    const float margin = style.FramePadding.x;
    ImGui::SetCursorPos(ImVec2(margin, margin));

    float xStart = ImGui::GetCursorPosX();
    float avail = ImGui::GetContentRegionAvail().x - margin;

    ImGui::Text("VIEWPORT");

    ImGui::SameLine();

    // The view modes, right aligned on the same row as the title. Same styling as the console's
    // filter tabs
    const int modes[4] = { ViewScene, ViewGame, ViewLighting, ViewRendering };
    const char* names[4] = { "Scene ", "Game ", "Lighting ", "Rendering" };

    float size = 0;
    for (int i = 0; i < 4; i++)
    {
        size += ImGui::CalcTextSize(names[i]).x;
    }

    // The spacing between the buttons
    size += style.ItemSpacing.x * 3.0f;

    float off = avail - size;
    if (off > 0.0f)
        ImGui::SetCursorPosX(xStart + off);

    for (int i = 0; i < 4; i++)
    {
        if (this->viewMode == modes[i])
        {
            ImGui::PushStyleColor(ImGuiCol_Text, Editor::ColorFromHex(Editor::PaletteAccent));
            ImGui::Text(names[i]);
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::TextDisabled(names[i]);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
            if (ImGui::IsItemClicked())
                this->SetViewMode(modes[i]);
        }

        if (i < 3)
            ImGui::SameLine();
    }

    // The overlay line is the footer, and the canvas gets the space between the header and it
    const ImVec2 region = ImGui::GetContentRegionAvail();
    const float footer = ImGui::GetFrameHeightWithSpacing();
    const ImVec2 canvas(region.x, region.y - footer);

    // The target is sized in physical pixels, because that is the resolution the scene is rendered
    // at. ImGui works in window coordinates, so the framebuffer scale is what turns one into the
    // other
    const ImVec2 density = ImGui::GetIO().DisplayFramebufferScale;
    const int width = (int)(canvas.x * density.x);
    const int height = (int)(canvas.y * density.y);

    if (canvas.x <= 0.0f || canvas.y <= 0.0f || width <= 0 || height <= 0)
        return;

    // The canvas is flush with the window, so the cursor goes back to the left edge
    ImGui::SetCursorPosX(0.0f);

    const ImTextureID id = this->OnTarget(width, height);
    if (id == 0)
        return;

    // The size the render side has now, which is what the status bar shows
    this->targetWidth = width;
    this->targetHeight = height;

    // The projection follows the canvas, not the window, so the scene is not stretched inside the
    // panel
    const float aspect = float(width) / float(height);

    // Scene mode draws with the panel's camera. Game mode draws with the game's, which the render side
    // answers for. A scene with no game camera keeps the panel's own view, and the overlay says so
    Math::mat4 view = cam.view;
    Math::mat4 projection = Math::perspfovrh(Math::deg2rad(this->fieldOfView), aspect, 0.01f, 1000.0f);

    if (this->viewMode == ViewGame)
    {
        Game::Entity camera = Game::Entity::Invalid();
        if (!this->OnGameView(camera, view, projection, aspect))
            camera = Game::Entity::Invalid();

        // The name is a lookup through the scene's records, so it is read again only when a different
        // camera becomes the active one
        if (camera != this->gameCamera)
        {
            this->gameCamera = camera;
            this->gameCameraName = "";
            if (camera != Game::Entity::Invalid())
            {
                Editor::Scene* scene = this->core->GetScene();
                this->gameCameraName = scene->GetEditable(scene->ToEditorEntity(camera)).name;
            }
        }
    }

    ImGui::Image(id, canvas);

    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    this->dragAnchor = Math::vec2((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);

    // The world basis in the top right corner of the canvas. It goes after the image, because the
    // draw list is ordered by the calls made in the frame and the canvas would cover it. Game view is
    // the game's view, so the editor draws nothing over its canvas
    if (this->viewMode != ViewGame)
        Editor::AxisIndicator::Draw(view);

    const bool rightClicked = hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    const bool middleClicked = hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle);

    // A camera drag does not need focus, because neither a right nor a middle click focuses a
    // window in ImGui. Without this the drag would only work in a panel that a left click had
    // already focused. Game view does not take the drag, because the view it shows is the game's and
    // not one the panel can turn
    if (this->viewMode != ViewGame && (rightClicked || middleClicked))
    {
        // The cursor is hidden and locked while the drag runs, so it does not stop at the edge of
        // the window
        this->cameraButton = rightClicked ? ImGuiMouseButton_Right : ImGuiMouseButton_Middle;
        this->core->SetCameraCapture(true);
        this->OnLockMouse(true);
        this->CenterMouse();
    }

    if (this->cameraButton >= 0)
    {
        // Escape lets go of the camera rather than running the tool chain on a frame the camera
        // drag owns the cursor on
        if (!ImGui::IsMouseDown(this->cameraButton) || ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            this->cameraButton = -1;
            this->core->SetCameraCapture(false);
            this->OnLockMouse(false);
        }
        else
        {
            // While the cursor is locked ImGui reports no position for it, so the movement comes
            // from the render side instead. What the movement does is the button's: the right
            // button turns in place, the middle button swings the camera around the pivot, and the
            // middle button with shift slides the view. Shift is read on the frame rather than
            // remembered from the press, so it can go down in the middle of a drag and take the
            // gesture over
            const Math::vec2 delta = this->OnMouseDelta();

            if (this->cameraButton == ImGuiMouseButton_Right)
                this->pendingInput.look = delta;
            else if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift))
                this->pendingInput.pan = delta;
            else
                this->pendingInput.orbit = {-delta.x, -delta.y};

            this->CenterMouse();
        }
    }

    // The wheel changes the distance from the camera to the point it orbits around. It is read on
    // the canvas whatever the panel's focus is, because scrolling is not a key the panel has to
    // own: the cursor being over the canvas is the whole condition. While a camera drag holds the
    // cursor ImGui reports no position for it, so the drag that began over the canvas stands in for
    // the hover
    const bool wheelOnCanvas = (hovered || this->cameraButton >= 0) && this->viewMode != ViewGame;
    if (wheelOnCanvas)
    {
        const float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f)
            this->pendingInput.zoom += wheel * ZoomSpeed;
    }

    // The tools read the canvas the way the panel does, through a view filled in here. The camera
    // drag owns the cursor, so a tool only sees the canvas while that drag is not running, and
    // escape is only a tool key while this panel has the keyboard.

    // The tools are read in three places below: the frame the view is filled with, the drop's
    // placement, and the handle pass that draws and reads this frame's drag
    Editor::ToolContext* tools = this->core->GetTools();

    Editor::ToolView toolView;
    toolView.canvasMin = Math::vec2(min.x, min.y);
    toolView.canvasSize = Math::vec2(canvas.x, canvas.y);
    toolView.view = view;
    toolView.projection = projection;
    toolView.frame = tools->GetFrame();
    toolView.hovered = hovered;
    // Escape is a tool key only outside a camera drag: under the drag it is the drag's own, and it
    // was already taken above
    toolView.escape = this->focused && this->cameraButton < 0 && ImGui::IsKeyPressed(ImGuiKey_Escape);
    toolView.ctrl = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    toolView.alt = ImGui::IsKeyDown(ImGuiKey_LeftAlt);

    const ImVec2& mousePos = ImGui::GetIO().MousePos;
    toolView.mouse = Math::vec2(mousePos.x, mousePos.y);
    if (this->cameraButton < 0)
    {
        // The viewport's mouse state follows one gesture that started over the canvas, the way the
        // old viewport's mouse capture did. A press that began in another panel is never seen by
        // the tools, so clicking the outliner cannot clear the selection and cannot start a drag
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            this->mouseDownOnCanvas = hovered;

        toolView.pressed = hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        // Game view is a viewer: the click picks, but nothing is held, so a drag cannot arm and the
        // tools cannot move the scene while the game is running
        toolView.held = this->viewMode != ViewGame && this->mouseDownOnCanvas && ImGui::IsMouseDown(ImGuiMouseButton_Left);
        toolView.released = this->mouseDownOnCanvas && ImGui::IsMouseReleased(ImGuiMouseButton_Left);

        // a button that is up is not holding any gesture, seen on the canvas or not
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            this->mouseDownOnCanvas = false;
    }

    // Read outside the block above: the tool needs the right click on the same frame the camera
    // drag takes the cursor, otherwise the cancel is swallowed by the drag
    toolView.rightPressed = rightClicked;

    // A file from the file browser turns into entities here. ImGui copies the payload into its own
    // buffer and holds it for the whole drag, so the path is read fresh every frame and only the
    // fact that a drop is in progress has to survive between them
    const bool overCanvas = ImGui::BeginDragDropTarget();
    if (overCanvas)
    {
        ImGui::PushStyleColor(ImGuiCol_DragDropTargetBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

        const ImGuiPayload* payload = ImGui::GetDragDropPayload();
        if (payload != nullptr && payload->IsDataType(Editor::FileDragPayload))
        {
            const ImVec2 mouse = ImGui::GetMousePos();
            Math::line ray;
            Math::point onPlane;
            Math::plane ground(Math::point(0.0f, 0.0f, 0.0f), Math::vector(0.0f, 1.0f, 0.0f));
            if (toolView.CanvasRay(Math::vec2(mouse.x, mouse.y), ray) && ground.intersect(ray, onPlane))
            {
                // The grid the author is dropping across is the lattice the placement comes to rest
                // on, so a file let go of over the canvas lands on its lines the way a dragged
                // entity does
                Math::vec3 point(onPlane.x, onPlane.y, onPlane.z);
                if (tools->SnapsToGrid())
                    point = tools->SnapToGrid(point);

                if (!this->fileDrop.isvalid())
                    this->BeginFileDrop((const char*)payload->Data, point);
                else
                    this->PreviewFileDrop(point);
            }

            // The point the commit writes is the one stored, because a release on a frame where the
            // ray does not reach the ground plane has no point of its own
            if (ImGui::AcceptDragDropPayload(Editor::FileDragPayload) != nullptr && this->fileDrop.isvalid())
                this->CommitFileDrop();
        }

        ImGui::PopStyleColor();
        ImGui::EndDragDropTarget();
    }

    // The preview instantiation belongs to the canvas. A drag that wanders off it, onto an outline
    // row or anywhere else, takes the instantiation back right away
    if (!overCanvas && this->fileDrop.isvalid() && ImGui::GetDragDropPayload() != nullptr)
        this->AbortFileDrop();

    if (this->fileDrop.isvalid() && ImGui::GetDragDropPayload() == nullptr)
        this->AbortFileDrop();

    // The tool's lines get queued here and go into the canvas when this frame renders. The handle
    // widget is an immediate ImGui control, so its pass both draws and reads this frame's drag, and
    // the tools act on what it reported right after. Game view draws nothing on the canvas and reads no
    // drag, so only the pick pass runs
    if (this->viewMode != ViewGame)
    {
        ImGuizmo::BeginFrame();
        tools->DrawView(toolView);
    }

    tools->UpdateView(toolView);

    ImGui::SetCursorPosX(margin);

    // The snapping is worth a word on this line, because whether a drag lands on something or goes
    // where the cursor puts it is exactly what an author is asking the canvas while they place.
    // Ctrl turns the sources the other way for as long as it is held, so the line answers for the
    // hold and says so
    const bool invertSnap = ImGui::GetIO().KeyCtrl;
    Util::String overlay;
    if (this->viewMode == ViewGame)
    {
        // A viewer has no gizmo to name, so the line says which camera it is looking through instead
        if (this->gameCamera == Game::Entity::Invalid())
            overlay.Format("Game   no camera   %.0f fps", ImGui::GetIO().Framerate);
        else
            overlay.Format("Game   %s   %.0f fps", this->gameCameraName.AsCharPtr(), ImGui::GetIO().Framerate);
    }
    else if (tools->SnapsToGrid() || tools->SnapsToIncrements() || invertSnap)
        overlay.Format("Viewport   %s gizmo   snap %s%s   %.0f fps", tools->FrameName(tools->GetFrame()),
            tools->SnapSourceName(invertSnap), invertSnap ? " (ctrl)" : "", ImGui::GetIO().Framerate);
    else
        overlay.Format("Viewport   %s gizmo   %.0f fps", tools->FrameName(tools->GetFrame()), ImGui::GetIO().Framerate);
    ImGui::TextUnformatted(overlay.AsCharPtr());

    const Game::TimeSource* time = Game::Time::GetTimeSource(TIMESOURCE_GAMEPLAY);

    // The keys are only read when the panel can navigate, but the input is gathered on every frame
    // regardless, because whatever the drag or the wheel put into pendingInput has to be consumed
    // in a panel that is not focused too, otherwise it would build up frame after frame. The camera
    // is driven on every frame the panel draws regardless: a focus can be started from a panel that
    // does not have the keyboard, and the frames the feedback loop takes to get there are drawn
    // here
    Editor::EditorCameraInput input = this->GatherInput();

    // Game view does not move the panel's camera, because the view it shows is the game's. The camera
    // stays where the author left it for when the panel comes back to scene mode
    if (this->viewMode != ViewGame)
        cam.Update(input, (float)time->frameTime);

    // The render side draws with the view this frame's state produced. The render happens after the
    // panels have drawn, so this is the frame the image shows
    this->OnRender(view, projection);
}

//------------------------------------------------------------------------------
/**
*/
void
ViewportPanel::OnFocus(bool focused)
{
    this->focused = focused;

    // The keys are read from ImGui, which stops reporting them when the window loses focus, so a
    // key that was held while the focus moved would otherwise stay in the input
    if (!focused && this->cameraButton < 0)
        this->pendingInput = Editor::EditorCameraInput();
}

//------------------------------------------------------------------------------
/**
    ImGui works in window coordinates and the warp takes window coordinates, so the anchor is used
    as it is. Re-centering every frame is what keeps the reported position inside the canvas, which
    is what keeps the panel owning the input while the cursor is locked.

    It goes through the render side rather than straight to the window, because the warp is reported
    as a cursor movement and would otherwise be read as the user moving the mouse.
*/
void
ViewportPanel::CenterMouse()
{
    this->OnWarpMouse(this->dragAnchor);
}

//------------------------------------------------------------------------------
/**
*/
bool
ViewportPanel::CanNavigate() const
{
    // Game view shows the game's camera, so the panel does not move it: the keys belong to the game.
    // Scene mode navigates when the panel has focus. The camera drag is not part of this, because
    // neither a right nor a middle click focuses a window
    return this->viewMode != ViewGame && this->focused;
}

//------------------------------------------------------------------------------
/**
    The keys are read from ImGui rather than from the input server, because the input server does
    not get them while ImGui owns the keyboard.
*/
Editor::EditorCameraInput
ViewportPanel::GatherInput()
{
    Editor::EditorCameraInput input = this->pendingInput;

    if (this->CanNavigate() || this->cameraButton >= 0)
    {
        const float speed = this->moveSpeed * (ImGui::IsKeyDown(ImGuiKey_LeftShift) ? ShiftSpeedFactor : 1.0f);

        if (ImGui::IsKeyDown(ImGuiKey_W))
            input.moveDir.z -= speed;
        if (ImGui::IsKeyDown(ImGuiKey_S))
            input.moveDir.z += speed;
        if (ImGui::IsKeyDown(ImGuiKey_A))
            input.moveDir.x -= speed;
        if (ImGui::IsKeyDown(ImGuiKey_D))
            input.moveDir.x += speed;
    }

    this->pendingInput = Editor::EditorCameraInput();
    return input;
}

//------------------------------------------------------------------------------
/**
*/
void
ViewportPanel::SetViewMode(int mode)
{
    if (mode == this->viewMode)
        return;

    this->viewMode = mode;
}

//------------------------------------------------------------------------------
/**
*/
const char*
ViewportPanel::ViewModeName(int mode) const
{
    switch (mode)
    {
    case ViewGame:
        return "game";
    case ViewLighting:
        return "lighting";
    case ViewRendering:
        return "rendering";
    default:
        break;
    }
    return "scene";
}

//------------------------------------------------------------------------------
/**
*/
bool
ViewportPanel::OnPlayTransition()
{
    const bool playing = this->core->GetScene()->IsPlaying();
    if (playing == this->wasPlaying)
        return false;

    this->wasPlaying = playing;

    if (ed_focus_game_on_play == nullptr || Core::CVarReadInt(ed_focus_game_on_play) <= 0)
        return false;

    if (playing)
    {
        this->autoViewMode = this->viewMode;
        this->SetViewMode(ViewGame);
        return true;
    }

    if (this->autoViewMode >= 0)
    {
        this->SetViewMode(this->autoViewMode);
        this->autoViewMode = -1;
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
/**
*/
int
ViewportPanel::TargetWidth() const
{
    return this->targetWidth;
}

//------------------------------------------------------------------------------
/**
*/
int
ViewportPanel::TargetHeight() const
{
    return this->targetHeight;
}

//------------------------------------------------------------------------------
/**
*/
float
ViewportPanel::FieldOfView() const
{
    return this->fieldOfView;
}

//------------------------------------------------------------------------------
/**
*/
void
ViewportPanel::CommandView(const Util::Array<Util::String>& args)
{
    if (args.IsEmpty())
    {
        this->core->Log(Editor::LogMessageType::N_SYSTEM, Util::String("view is ") + this->ViewModeName(this->viewMode));
        return;
    }

    if (args[0] == "game")
        this->SetViewMode(ViewGame);
    else if (args[0] == "lighting")
        this->SetViewMode(ViewLighting);
    else if (args[0] == "rendering")
        this->SetViewMode(ViewRendering);
    else if (args[0] == "scene")
        this->SetViewMode(ViewScene);
    else
        this->core->Log(Editor::LogMessageType::N_WARNING, "view is one of: scene, game, lighting, rendering");
}

//------------------------------------------------------------------------------
/**
*/
void
ViewportPanel::CommandFov(const Util::Array<Util::String>& args)
{
    if (args.IsEmpty())
    {
        Util::String msg;
        msg.Format("fov is %.1f degrees", this->fieldOfView);
        this->core->Log(Editor::LogMessageType::N_SYSTEM, msg);
        return;
    }

    const float fov = args[0].AsFloat();
    if (fov <= 0.0f || fov >= 170.0f)
    {
        this->core->Log(Editor::LogMessageType::N_WARNING, "fov has to be between 0 and 170 degrees");
        return;
    }

    this->fieldOfView = fov;
}

//------------------------------------------------------------------------------
/**
*/
void
ViewportPanel::CommandSpeed(const Util::Array<Util::String>& args)
{
    if (args.IsEmpty())
    {
        Util::String msg;
        msg.Format("speed is %.1f units per second", this->moveSpeed);
        this->core->Log(Editor::LogMessageType::N_SYSTEM, msg);
        return;
    }

    const float speed = args[0].AsFloat();
    if (speed <= 0.0f)
    {
        this->core->Log(Editor::LogMessageType::N_WARNING, "speed has to be greater than 0");
        return;
    }

    this->moveSpeed = speed;
}

//------------------------------------------------------------------------------
/**
*/
void
ViewportPanel::CommandFocus(const Util::Array<Util::String>& args)
{
    (void)args;

    Editor::Scene* scene = this->core->GetScene();
    const Util::Array<Game::Entity>& entities = this->core->GetSelection()->GetSelection();
    if (entities.IsEmpty())
    {
        this->core->Log(Editor::LogMessageType::N_WARNING, "focus needs a selection");
        return;
    }

    // The bounds are the ones picking uses, so the focus goes to where the entity is drawn rather
    // than to the origin it was authored with
    Math::bbox bounds;
    Game::Entity first = Game::Entity::Invalid();
    IndexT numBoxes = 0;

    for (IndexT i = 0; i < entities.Size(); i++)
    {
        Math::bbox box;
        if (!Editor::ToolContext::WorldBounds(scene, entities[i], box))
            continue;

        if (numBoxes == 0)
        {
            bounds = box;
            first = entities[i];
        }
        else
            bounds.extend(box);

        numBoxes++;
    }

    if (numBoxes == 0)
    {
        this->core->Log(Editor::LogMessageType::N_WARNING, "nothing in the selection has bounds to focus on");
        return;
    }

    // The half diagonal is the radius of the sphere the box fits in, and the distance that puts
    // that sphere on the edge of the view is the one the camera has to stand back at
    const Math::vec3 center = bounds.center().vec;
    const float radius = Math::length(bounds.extents());
    float distance = radius / Math::sin(Math::deg2rad(this->fieldOfView) * 0.5f) * FocusMargin;
    if (distance < MinFocusDistance)
        distance = MinFocusDistance;

    // A focus is a pivot: the selection becomes the point the camera orbits around, and the camera
    // stands back from it at the distance the framing worked out. The feedback loop is what takes
    // it there over the frames after this one, which is what makes a focus glide rather than land
    Editor::EditorCamera& cam = scene->GetCamera();
    cam.SetPivot(center, distance);

    Util::String msg;
    if (numBoxes == 1)
        msg.Format("Focus on %s", scene->GetEditable(first).name.AsCharPtr());
    else
        msg.Format("Focus on %d entities", (int)numBoxes);
    this->core->Log(Editor::LogMessageType::N_SYSTEM, msg);
}

//------------------------------------------------------------------------------
/**
*/
void
ViewportPanel::CommandReset(const Util::Array<Util::String>& args)
{
    (void)args;

    // The camera is the active scene's, so reset only resets the tab that is showing
    this->core->GetScene()->ResetCamera();

    this->fieldOfView = DefaultFieldOfView;
    this->moveSpeed = DefaultMoveSpeed;
    this->core->Log(Editor::LogMessageType::N_SYSTEM, "Viewport camera reset");
}

//------------------------------------------------------------------------------
/**
    The command is the drop's state: it is the same one the macro holds, and the panels read what it
    made. The roots are worked out again here each frame rather than kept on the panel, because a
    preview is gone at the start of the next frame and the roots have to be written again anyway.
*/
Util::Array<Game::Entity>
ViewportPanel::FileDropRoots() const
{
    Util::Array<Game::Entity> roots;
    if (!this->fileDrop.isvalid() || !this->fileDrop->scene.isvalid())
        return roots;

    Ptr<Editor::Scene> scene = this->fileDrop->scene;
    const Game::ComponentId position = Game::GetComponentId<Game::Position>();
    for (IndexT i = 0; i < this->fileDrop->created.Size(); i++)
    {
        const Game::Entity entity = this->fileDrop->created[i];
        if (scene->GetParent(entity) != Game::Entity::Invalid())
            continue;
        if (!scene->HasComponent(entity, position))
            continue;

        roots.Append(entity);
    }

    return roots;
}

//------------------------------------------------------------------------------
/**
*/
void
ViewportPanel::BeginFileDrop(const char* path, const Math::vec3& onPlane)
{
    Ptr<Editor::Scene> scene = this->core->GetScene();
    if (!scene.isvalid())
        return;

    Ptr<Editor::EditCommandManager> manager = scene->GetCommandManager();
    if (!manager.isvalid())
        return;

    // The panel holds the same command the macro gets, which is what lets the frames after this one
    // know what the run made. A file that cannot be read leaves nothing behind, and the drop is
    // then a macro that the release closes without an entry under it
    this->fileDrop = Editor::InstantiatePrefabCommand::Create();
    this->fileDrop->scene = scene;
    this->fileDrop->path = path;

    // The macro stays open for the whole drag. It is what makes the instantiate and the placement
    // one undo entry, and what lets a drag that ends off the canvas take the instantiate back with
    // one undo
    manager->BeginMacro("Instantiate");
    manager->Execute(this->fileDrop);

    this->PreviewFileDrop(onPlane);
}

//------------------------------------------------------------------------------
/**
    The authored positions are the ones the file brought with it, because a preview writes the sim
    world's instance and leaves the authoring world alone, so reading them back gives the same
    values as the first frame. The drag moves everything by the cursor's distance from the first
    root, which is what keeps the file's own layout.
*/
void
ViewportPanel::PreviewFileDrop(const Math::vec3& onPlane)
{
    this->fileDropPoint = onPlane;

    if (!this->fileDrop.isvalid() || !this->fileDrop->scene.isvalid())
        return;

    Ptr<Editor::Scene> scene = this->fileDrop->scene;
    const Util::Array<Game::Entity> roots = this->FileDropRoots();
    if (roots.IsEmpty())
        return;

    Game::World* world = scene->GetAuthoringWorld();
    const Game::ComponentId position = Game::GetComponentId<Game::Position>();
    const Math::vec3 origin = world->GetComponent<Game::Position>(roots[0]);
    const Math::vec3 delta = this->fileDropPoint - origin;

    for (IndexT i = 0; i < roots.Size(); i++)
    {
        const Math::vec3 start = world->GetComponent<Game::Position>(roots[i]);
        const Game::Position pos(start + delta);
        scene->PreviewComponent(roots[i], position, &pos);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ViewportPanel::CommitFileDrop()
{
    if (!this->fileDrop.isvalid() || !this->fileDrop->scene.isvalid())
        return;

    Ptr<Editor::Scene> scene = this->fileDrop->scene;
    Ptr<Editor::EditCommandManager> manager = scene->GetCommandManager();
    if (!manager.isvalid())
        return;

    const Util::Array<Game::Entity> roots = this->FileDropRoots();
    if (!roots.IsEmpty())
    {
        Game::World* world = scene->GetAuthoringWorld();
        const Game::ComponentId position = Game::GetComponentId<Game::Position>();
        const Math::vec3 origin = world->GetComponent<Game::Position>(roots[0]);
        const Math::vec3 delta = this->fileDropPoint - origin;

        for (IndexT i = 0; i < roots.Size(); i++)
        {
            const Math::vec3 start = world->GetComponent<Game::Position>(roots[i]);
            const Game::Position pos(start + delta);

            Ptr<Editor::SetComponentCommand> command = Editor::SetComponentCommand::Create();
            command->scene = scene;
            command->entity = roots[i];
            command->component = position;
            command->newValue = Util::Blob(&pos, sizeof(pos));
            manager->Execute(command);
        }
    }

    manager->EndMacro();
    this->fileDrop = nullptr;
}

//------------------------------------------------------------------------------
/**
    The command holds the scene it ran against, so a drop is finished off against that one rather
    than against whichever tab happens to be showing.
*/
void
ViewportPanel::AbortFileDrop()
{
    if (this->fileDrop.isvalid() && this->fileDrop->scene.isvalid())
    {
        Ptr<Editor::EditCommandManager> manager = this->fileDrop->scene->GetCommandManager();
        if (manager.isvalid())
        {
            manager->EndMacro();

            // A file that could not be read ran no command, so there is nothing under the macro to
            // take back, and undoing here would take whatever the author did before the drop
            if (!this->fileDrop->created.IsEmpty())
                manager->Undo();
        }
    }

    this->fileDrop = nullptr;
}

} // namespace Editor
//------------------------------------------------------------------------------
