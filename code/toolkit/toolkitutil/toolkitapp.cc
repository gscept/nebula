//------------------------------------------------------------------------------
//  toolkitapp.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "toolkitutil/toolkitapp.h"

namespace ToolkitUtil
{
using namespace IO;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
ToolkitApp::ToolkitApp() :
    toolkitVersion(NEBULA_TOOLKIT_VERSION),
    waitForKey(false),
#if __WIN32__
    platform(Platform::Win32)
#elif __LINUX__
    platform(Platform::Linux)
#endif
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
ToolkitApp::Open()
{
    if (ConsoleApplication::Open())
    {
        // need to disable ZIP file system in tools!
        IoServer::Instance()->SetArchiveFileSystemEnabled(false);

		// add toolkit handler for structured logging
		this->handler = ToolkitUtil::ToolkitConsoleHandler::Create();
		IO::Console::Instance()->AttachHandler(this->handler.cast<IO::ConsoleHandler>());
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
ToolkitApp::Close()
{
	IO::Console::Instance()->RemoveHandler(this->handler.cast<IO::ConsoleHandler>());
	this->handler = nullptr;
	ConsoleApplication::Close();
}

//------------------------------------------------------------------------------
/**
*/
bool
ToolkitApp::ParseCmdLineArgs()
{
    bool help = this->args.GetBoolFlag("-help");
    if (help)
    {
        this->ShowHelp();
        return false;
    }
    if(this->args.HasArg("-platform"))
    {
        this->platform = Platform::FromString(this->args.GetString("-platform"));
    }
    this->waitForKey = this->args.GetBoolFlag("-waitforkey");
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
ToolkitApp::SetupProjectInfo()
{
    // read the projectinfo.xml file
    ProjectInfo::Result res = this->projectInfo.Setup();
    if (res != ProjectInfo::Success)
    {
        n_printf("%s\n", this->projectInfo.GetErrorString(res).AsCharPtr());
        this->SetReturnCode(10);
        return false;
    }

    // gather the relevant paths from the project info object
    if (!this->projectInfo.HasPlatform(this->platform))
    {
        n_printf("ERROR: platform '%s' not defined in projectinfo.xml file!\n",
            Platform::ToString(this->platform).AsCharPtr());
        this->SetReturnCode(10);
        return false;
    }

    // prepare generic info about project
    this->projectInfo.SetCurrentPlatform(this->platform);
    AssignRegistry::Instance()->SetAssign(Assign("src", this->projectInfo.GetAttr("SrcDir")));
    AssignRegistry::Instance()->SetAssign(Assign("dst", this->projectInfo.GetAttr("DstDir")));
    AssignRegistry::Instance()->SetAssign(Assign("int", this->projectInfo.GetAttr("IntDir")));

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
ToolkitApp::ShowHelp()
{
    n_printf("Generic ToolkitApp help text. FIXME!!!\n");
}

} // namespace ToolkitUtil
