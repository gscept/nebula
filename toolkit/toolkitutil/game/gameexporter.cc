//------------------------------------------------------------------------------
//  gameexporter.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "game/gameexporter.h"
#include "io/ioserver.h"
#include "levelexporter.h"

using namespace IO;
using namespace Util;

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::GameExporter, 'GAEX', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
GameExporter::GameExporter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
GameExporter::~GameExporter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void 
GameExporter::Open()
{
    AssetProcessorBase::Open();
}

//------------------------------------------------------------------------------
/**
*/
void 
GameExporter::Close()
{
    AssetProcessorBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void 
GameExporter::ProcessAll()
{
    String projectFolder = "proj:";
    
    Ptr<ToolkitUtil::ToolkitConsoleHandler> console = ToolkitUtil::ToolkitConsoleHandler::Instance();
    console->Clear();

    Ptr<LevelExporter> levelExporter = LevelExporter::Create();
    this->Progress(5, "levels");
    levelExporter->SetLogger(this->logger);
    levelExporter->Open();
    levelExporter->ProcessAll();
    levelExporter->Close();
}

} // namespace ToolkitUtil