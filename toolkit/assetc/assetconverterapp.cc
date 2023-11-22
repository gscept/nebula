//------------------------------------------------------------------------------
//  assetconverterapp.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "toolkitutil/model/import/fbx/nfbxexporter.h"
#include "toolkitutil/model/modelutil/modeldatabase.h"
#include "assetconverterapp.h"
#include "io/assignregistry.h"
#include "core/coreserver.h"
#include "io/textreader.h"
#include "asset/assetexporter.h"
#include "io/console.h"
#include "nflatbuffer/flatbufferinterface.h"
#ifdef WIN32
#include "io/win32/win32consolehandler.h"
#else
#include "io/posix/posixconsolehandler.h"
#endif

#define NEBULA_TEXT_IMPLEMENT
#include "toolkit-common/text.h"


#define PRECISION 1000000

using namespace IO;
using namespace Util;
using namespace ToolkitUtil;
using namespace Base;

namespace Toolkit
{
//------------------------------------------------------------------------------
/**
*/
AssetConverterApp::AssetConverterApp()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AssetConverterApp::~AssetConverterApp()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void 
AssetConverterApp::Run()
{
    if (this->args.HasArg("-help") || !this->args.HasArg("-dir"))
    {
        this->ShowHelp();
        return;
    }

    bool success = true;

    // setup the project info object
    if (success && !this->SetupProjectInfo())
    {
        success = false;
        this->SetReturnCode(-1);
    }

    // parse command line args
    if (success && !this->ParseCmdLineArgs())
    {
        success = false;
        this->SetReturnCode(-1);
    }

    Flat::FlatbufferInterface::Init();

    this->modelDatabase = ToolkitUtil::ModelDatabase::Create();
    this->modelDatabase->Open();

    Ptr<AssetExporter> exporter = AssetExporter::Create();
    
    Util::String dir = this->args.GetString("-dir");
    ExporterBase::ExportFlag exportFlag = ExporterBase::All;
    uint32_t mode = AssetExporter::All;
    
    if (this->args.HasArg("-mode"))
    {
        mode = 0;
        Util::String exportMode = this->args.GetString("-mode");
        Util::Array<Util::String> modeFlags = exportMode.Tokenize(",");
        if (modeFlags.Find("fbx")) mode |= AssetExporter::FBX;
        if (modeFlags.Find("model")) mode |= AssetExporter::Models;
        if (modeFlags.Find("surface")) mode |= AssetExporter::Surfaces;
        if (modeFlags.Find("texture")) mode |= AssetExporter::Textures;
        if (modeFlags.Find("physics")) mode |= AssetExporter::Physics;
        if (modeFlags.Find("gltf")) mode |= AssetExporter::GLTF;
    }
    
    IO::AssignRegistry::Instance()->SetAssign(Assign("home","proj:"));
    IO::AssignRegistry::Instance()->SetAssign(Assign("src", "proj:work"));

    exporter->Open();
    exporter->SetForce(true);
    exporter->SetExportMode(mode);
    
    exporter->SetExportFlag(ExporterBase::Dir);
    exporter->SetPlatform(this->platform);
    exporter->SetProgressPrecision(PRECISION);
    exporter->SetProgressMinMax(0, PRECISION);    
    exporter->ExportDir(dir);
    exporter->Close();

    // if we have any errors, set the return code to be errornous   
    if (exporter->HasErrors()) this->SetReturnCode(-1);
    this->modelDatabase->Close();
    this->modelDatabase = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void 
AssetConverterApp::ShowHelp()
{
    n_printf("Nebula asset converter.\n"
        "(C) 2020 Individual contributors, see AUTHORS file.\n");
    n_printf("Usage assetc [args] -dir [path to assetsfolder]\n"
            "-help         --display this help\n"
             "-mode         --selects type to batch (fbx,model,texture,surface,physics,gltf) defaults to all");
}

} // namespace Toolkit

