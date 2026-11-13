#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::ProjectLocator

    Resolves the project root and the toolkit (nebula root) directory.

    The project info file always lives in the project root. Project
    identity is therefore a property of where you are.

    Resolution order, highest priority first:

    1. an environment variable (NEBULA_PROJECT / NEBULA_TOOLKIT)
    2. walking up from the current working directory looking for the project
       info file
    3. the value embedded by the build system (NEBULA_PROJECT_ROOT /
       NEBULA_TOOLKIT_ROOT), which is only needed when the tool is invoked
       from a build directory that is not inside the project

    An explicit command line override outranks all of these, but is handled by
    the caller, since the locator must not depend on the application layer.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"

//------------------------------------------------------------------------------
namespace IO
{
class ProjectLocator
{
public:
    /// name of the project info file, which is always in the project root
    static const char* ProjectInfoFileName;

    /// resolve the project root, empty string if it cannot be found
    static Util::String GetProjectRoot();
    /// resolve the toolkit (nebula root) directory, empty if not found
    static Util::String GetToolkitRoot();
    /// walk up from startDir looking for the project info file
    static Util::String FindProjectRoot(const Util::String& startDir);
};

} // namespace IO
//------------------------------------------------------------------------------
