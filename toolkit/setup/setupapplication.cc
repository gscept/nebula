//------------------------------------------------------------------------------
//  setupapplication.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "setupapplication.h"
#include "io/projectlocator.h"

using namespace ToolkitUtil;

namespace Tools
{
//------------------------------------------------------------------------------
/**
*/
SetupApplication::SetupApplication()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
SetupApplication::~SetupApplication()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool 
SetupApplication::Open()
{
    bool retval = ToolkitApp::Open();
    this->DoWork();
    return retval;
}

//------------------------------------------------------------------------------
/**
*/
void 
SetupApplication::DoWork()
{
    // Nothing is stored here any more. The project root is wherever the project
    // info file is, and the toolkit root comes from the build system or the
    // environment, so this tool only reports what the current project resolves
    // to.
    Util::String projectRoot = IO::ProjectLocator::GetProjectRoot();
    Util::String toolkitRoot = IO::ProjectLocator::GetToolkitRoot();

    n_printf("Project root : %s\n", projectRoot.IsEmpty() ? "(not resolved)" : projectRoot.AsCharPtr());
    n_printf("Toolkit root   : %s\n", toolkitRoot.IsEmpty() ? "(not resolved)" : toolkitRoot.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
bool 
SetupApplication::ParseCmdLineArgs()
{
    return ToolkitApp::ParseCmdLineArgs();
}

//------------------------------------------------------------------------------
/**
*/
bool 
SetupApplication::SetupProjectInfo()
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void 
SetupApplication::ShowHelp()
{
    n_printf("NebulaT Setup application.\n"
        "(C) 2012-2016 Individual contributors, see AUTHORS file.\n");
    n_printf("-help         --display this help\n"
        "-nody      --nody path override"
        "-working   --nebula working folder (where bins are located)\n"
        "-project   --nebula project trunk (defaults to the directory the project info file is in)\n"
        "-platform     --export platform");
}
} // namespace Tools