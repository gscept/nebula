//------------------------------------------------------------------------------
//  @file assetsfeatureunit.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editorassets/assetsfeatureunit.h"
#include "editorassets/assetbrowserpanel.h"
#include "editorassets/assetdatabase.h"
#include "editorassets/asseteditorpanel.h"
#include "editorassets/assetpicker.h"
#include "editorassets/importpanel.h"
#include "editorassets/batcherpanel.h"
#include "editorassets/livebatcher.h"
#include "editorassets/resourcefield.h"
#include "neweditor/api/editorfeature.h"
#include "io/assignregistry.h"
#include "io/ioserver.h"
#include "io/path.h"
#include "toolkitutil/projectinfo.h"

namespace Editor
{

__ImplementClass(Editor::AssetsFeatureUnit, 'EASF', Game::FeatureUnit);

//------------------------------------------------------------------------------
/**
*/
AssetsFeatureUnit::AssetsFeatureUnit()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AssetsFeatureUnit::~AssetsFeatureUnit()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    An assign a game already set is the game's, so only a missing one is set here.
*/
static void
SetAssignIfMissing(const char* name, const Util::String& path)
{
    IO::AssignRegistry* assigns = IO::AssignRegistry::Instance();
    if (!assigns->HasAssign(name))
        assigns->SetAssign(IO::Assign(name, path));
}

//------------------------------------------------------------------------------
/**
    The status bar says how many batcher jobs are left while there are any, which is what tells an
    author an asset is still exporting
*/
static void
AppendBatcherStatus(Util::String& out)
{
    if (LiveBatcher::IsBusy())
        out.Format("batching %d", LiveBatcher::WorkCounter.counter);
}

//------------------------------------------------------------------------------
/**
*/
void
AssetsFeatureUnit::OnActivate()
{
    Game::FeatureUnit::OnActivate();

    // The folders an asset is authored and imported in. 'src' is where the files an asset is made
    // from are copied to, and it is the work folder until a project says otherwise
    SetAssignIfMissing("work", "proj:work");
    SetAssignIfMissing("assets", "work:assets");
    SetAssignIfMissing("src", "proj:work");

    // The intermediate folder is the project's, the import writes what it converts there
    ToolkitUtil::ProjectInfo projectInfo;
    if (projectInfo.Setup() == ToolkitUtil::ProjectInfo::Success)
    {
        SetAssignIfMissing("int", projectInfo.GetAttr("IntermediateDir"));
        IO::CreateDirectory("int:");
    }
    else
    {
        n_warning("Editor assets: the project info could not be read, so there is no 'int:' folder to import into\n");
    }

    // A URN, folder:file:type, names its work file below this root
    IO::Path::SetWorkRoot("proj:work");

    LiveBatcher::Setup();

    // The index lives in the intermediate folder, a project without one has a browser that says so
    this->database = AssetDatabase::Create();
    this->database->Setup();
    this->picker = AssetPicker::Create();

    EditorFeatureRegistry::RegisterPanel(AssetBrowserPanel::Create());
    EditorFeatureRegistry::RegisterPanel(AssetEditorPanel::Create());
    EditorFeatureRegistry::RegisterPanel(ImportPanel::Create());
    EditorFeatureRegistry::RegisterPanel(BatcherPanel::Create());
    EditorFeatureRegistry::RegisterModal(Util::Delegate<void()>::FromMethod<AssetPicker, &AssetPicker::Draw>(this->picker));
    EditorFeatureRegistry::RegisterStatusNumbers(Util::Delegate<void(Util::String&)>::FromFunction<&AppendBatcherStatus>());

    // the inspector picks the assets a component refers to
    SetupResourceFields();
}

//------------------------------------------------------------------------------
/**
    The editor is removed first, so the panels are gone by now
*/
void
AssetsFeatureUnit::OnDeactivate()
{
    DiscardResourceFields();
    this->picker = nullptr;
    if (this->database.isvalid())
    {
        this->database->Discard();
        this->database = nullptr;
    }
    LiveBatcher::Discard();
    Game::FeatureUnit::OnDeactivate();
}

//------------------------------------------------------------------------------
/**
*/
void
AssetsFeatureUnit::OnBeginFrame()
{
    Game::FeatureUnit::OnBeginFrame();
    LiveBatcher::Update();
    if (this->database.isvalid())
        this->database->Update();
    ImportPanel::Update();
}

} // namespace Editor
