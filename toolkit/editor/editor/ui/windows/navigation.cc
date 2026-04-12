//------------------------------------------------------------------------------
//  navigation.cc
//  (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "navigation.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/ui/uimanager.h"
#include "editor/tools/selectiontool.h"
#include "editor/cmds.h"
#include "physics/debugui.h"
#include "editor/ui/windowserver.h"
#include "editor/ui/windows/scene.h"
#include "system/library.h"

namespace
{
using NavigationFeatureRenderUiFn = void (*)(Graphics::GraphicsEntityId camera);

NavigationFeatureRenderUiFn
LoadNavigationFeatureRenderUi()
{
    static NavigationFeatureRenderUiFn renderUi = nullptr;
    static Base::Library* library = nullptr;
    static bool attemptedLoad = false;
    if (attemptedLoad)
        return renderUi;

    attemptedLoad = true;

    Util::String libraryFile;
#if __WIN32__
    libraryFile = "navigationfeaturemodule.dll";
#else
    libraryFile = "libnavigationfeaturemodule.so";
#endif

    Util::String libraryPath = Util::String::Sprintf("%s/%s", NEBULA_BINARY_FOLDER, libraryFile.AsCharPtr());
    library = new System::Library();
    library->SetPath(IO::URI(libraryPath));
    if (!library->Load())
        return nullptr;

    renderUi = reinterpret_cast<NavigationFeatureRenderUiFn>(library->GetExport("NebulaNavigationFeatureRenderUI"));
    if (renderUi == nullptr)
    {
        library->Close();
        delete library;
        library = nullptr;
    }

    return renderUi;
}
}

using namespace Editor;

namespace Presentation
{
__ImplementClass(Presentation::Navigation, 'PtNa', Presentation::BaseWindow);

//------------------------------------------------------------------------------
/**
*/
bool
EnsureNavigationUiHookLoaded()
{
    return LoadNavigationFeatureRenderUi() != nullptr;
}

//------------------------------------------------------------------------------
/**
*/
Navigation::Navigation()
{
    this->additionalFlags = ImGuiWindowFlags_(ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
}

//------------------------------------------------------------------------------
/**
*/
Navigation::~Navigation()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Navigation::Run(SaveMode save)
{
    if (this->defaultCamera == Graphics::GraphicsEntityId::Invalid())
    {
        Ptr<Scene> sceneWindow = Presentation::WindowServer::Instance()->GetWindow("Scene View").downcast<Presentation::Scene>();
        if (sceneWindow.isvalid())
        {
            this->defaultCamera = ViewGetCamera(sceneWindow->viewPort.GetView());
        }
    }
    if (this->defaultCamera == Graphics::GraphicsEntityId::Invalid())
    {
        return;
    }

    NavigationFeatureRenderUiFn renderUi = LoadNavigationFeatureRenderUi();
    if (renderUi != nullptr)
    {
        renderUi(this->defaultCamera);
        this->missingHookWarningShown = false;
    }
    else
    {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Navigation runtime module is unavailable.");
        ImGui::TextUnformatted("Expected module: libnavigationfeaturemodule.so in deploy folder.");

        if (!this->missingHookWarningShown)
        {
            n_warning("Editor Navigation window: failed to load runtime hook from navigationfeaturemodule.");
            this->missingHookWarningShown = true;
        }
    }
}

} // namespace Presentation
