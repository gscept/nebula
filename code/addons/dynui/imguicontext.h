#pragma once
//------------------------------------------------------------------------------
/**
    @class Dynui::ImguiRenderer
    
    Nebula renderer for  the IMGUI dynamic UI library.
    
    @copyright
    (C) 2012-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/texture.h"
#include "coregraphics/buffer.h"
#include "coregraphics/resourcetable.h"
#include "input/inputevent.h"
#include "coregraphics/pipeline.h"
#include "graphics/graphicscontext.h"
#include "coregraphics/vertexlayout.h"
#include "imguiinputhandler.h"
#include "memory/arenaallocator.h"
#include "frame/framecode.h"
#include "util/delegate.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

struct ImDrawData;
struct ImFont;
struct ImVec2;
namespace Dynui
{

extern ImGuiKey NebulaToImguiKeyCodes[];
extern Util::Array<Util::String> ImguiDragAndDropFiles;
extern const char* EditorUIPath;

/// The platform side of a secondary ImGui viewport
struct ImGuiWindowHandle
{
    CoreGraphics::WindowId wnd;

    /// The frame the window was last moved or resized by ImGui itself. The window system reports
    /// those changes back as move and resize events, but late, after ImGui has already moved the
    /// window further. Taken as moves the user made, they put the window back where it was a few
    /// frames ago, which is what makes a dragged window jump. They are ignored for a few frames
    int ownPosFrame = -1000;
    int ownSizeFrame = -1000;
};

/// how many frames after ImGui moved or resized a window its own change can still come back
constexpr int ImGuiWindowEchoFrames = 3;

struct ImguiTextureId
{
    CoreGraphics::TextureId nebulaHandle = 0;
    uint layer : 8 = 0;
    uint mip : 4 = 0;
    uint useAlpha : 1 = 1;
    uint useRange : 1 = 0;
    float rangeMin, rangeMax;
    uint red : 1 = 1;
    uint green : 1 = 1;
    uint blue : 1 = 1;
    uint alpha : 1 = 1;
    uint splat : 1 = 0;
    uint pointFilter : 1 = 0;
    uint tonemap : 1 = 0;       // tonemap HDR (float and BC6H) textures instead of clipping them, LDR textures are unaffected
    float exposure = 1.0f;      // linear exposure scale used when tonemapping
};

extern Ids::IdAllocator<ImguiTextureId> ImguiTextureIdAllocator;

/// Allocate imgui texture
Ids::Id32 AllocateImguiTextureId(const ImguiTextureId& data);
/// Set imgui texture data
void SetImguiTextureIdData(Ids::Id32 id, const ImguiTextureId& data);
/// Deallocate imgui texture
void DeallocateImguiTextureId(Ids::Id32 id);


extern ImFont* ImguiFont;
extern ImFont* ImguiBoldFont;
extern ImFont* ImguiItFont;

class ImguiContext : public Graphics::GraphicsContext
{
    __DeclarePluginContext();
public:
    /// constructor
    ImguiContext();
    /// destructor
    virtual ~ImguiContext();

    /// Create context
    static void Create();
    /// Discard context
    static void Discard();

    /// set the screen dimensions to use when rendering the UI (all vertices will be mapped to these values)
    static void SetRectSize(SizeT width, SizeT height);
    
    /// handle event
    static bool HandleInput(const Input::InputEvent& event);

    /// the keys a text field did not take. The callback answers whether it took one, and the event
    /// does not go on to the input server when it did
    static void SetKeyCallback(Util::Delegate<bool(Input::Key::Code, bool, Input::Modifier::Code)> callback);

    /// called if the window size has changed
    static void OnViewportResized(const uint framescriptHash, SizeT width, SizeT height);
    /// called before frame
    static void NewFrame(const Graphics::FrameContext& ctx);

    /// called after frame
    static void EndFrame(const Graphics::FrameContext& ctx);

    /// draw and show a window ImGui made for a window dragged out of the main window. Not for the main window
    static void PresentWindow(const CoreGraphics::WindowId window, IndexT frameIndex, IndexT bufferIndex);

    /// get the unscaled base style, call ApplyBaseStyle after modifying it
    static ImGuiStyle& GetBaseStyle();
    /// get the content scale applied on top of the base style
    static float GetStyleScale();
    /// copy the base style into the active ImGui style, scaled by the content scale
    static void ApplyBaseStyle();

private:
    static void RecoverImGuiContextErrors();

    /// the keys a text field did not take
    static Util::Delegate<bool(Input::Key::Code, bool, Input::Modifier::Code)> keyCallback;
};

/// Helper function to recreate the ImGUI close button used for windows and tabs
void ImGuiCloseButton(bool& toggle, int id);
/// Helper function for toggle buttons
bool ImGuiToggleButton(const char* label, bool toggle);
/// Helper function to produce a spinner
void ImGuiSpinner(const char* label, float radius = 10.0f, float thickness = 3.0f);

} // namespace Dynui
