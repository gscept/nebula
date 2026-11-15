//------------------------------------------------------------------------------
//  projectlocator.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/projectlocator.h"
#include "io/fswrapper.h"
#include "system/environment.h"

namespace IO
{
using namespace Util;

const char* ProjectLocator::ProjectInfoFileName = "projectinfo.json";

//------------------------------------------------------------------------------
/**
*/
String
ProjectLocator::FindProjectRoot(const String& startDir)
{
    String dir = startDir;
    dir.ConvertBackslashes();
    dir.TrimRight("/");
    while (!dir.IsEmpty())
    {
        if (IO::FSWrapper::FileExists(String::AppendPath(dir, String(ProjectLocator::ProjectInfoFileName))))
        {
            dir.TrimRight("/");
            return dir;
        }

        String parent = dir.ExtractDirName();
        if (parent.IsEmpty() || parent == dir)
        {
            break;
        }
        dir = parent;
    }
    return String();
}

//------------------------------------------------------------------------------
/**
*/
String
ProjectLocator::GetProjectRoot()
{
    // 1) an explicit environment override
    if (System::Environment::Exists("NEBULA_PROJECT"))
    {
        String envRoot = System::Environment::Read("NEBULA_PROJECT");
        envRoot.ConvertBackslashes();
        envRoot.TrimRight("/");
        return envRoot;
    }

    // 2) the project we are standing in
    String projectRoot = FindProjectRoot(FSWrapper::GetCurrentDirectory());
    if (!projectRoot.IsEmpty())
    {
        return projectRoot;
    }

    // 3) the value embedded by the build system, as a fallback for tools
    //    invoked from a build directory, which is not inside the project
    #if NEBULA_DEBUG
    #ifdef NEBULA_PROJECT_ROOT
    return String(NEBULA_PROJECT_ROOT);
    #endif
    #endif

    return String();
}

//------------------------------------------------------------------------------
/**
*/
String
ProjectLocator::GetToolkitRoot()
{
    // 1) an explicit environment override
    if (System::Environment::Exists("NEBULA_TOOLKIT"))
    {
        return System::Environment::Read("NEBULA_TOOLKIT");
    }

    // 2) the value embedded by the build system for this project
    #ifdef NEBULA_TOOLKIT_ROOT
    return String(NEBULA_TOOLKIT_ROOT);
    #endif

    // 3) not derivable, the toolkit is a separate checkout
    return String();
}

} // namespace IO
//------------------------------------------------------------------------------
