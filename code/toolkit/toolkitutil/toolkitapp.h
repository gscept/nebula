#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::ToolkitApp
    
    Base class for typical toolkit command line tools. Only provides a
    few helper methods, the actual application still must provide the
    complete Run() method.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "app/consoleapplication.h"
#include "toolkitutil/projectinfo.h"
#include "toolkitutil/platform.h"
#include "toolkitutil/logger.h"
#include "toolkitutil/toolkitversion.h"
#include "toolkitutil/toolkitconsolehandler.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class ToolkitApp : public App::ConsoleApplication
{
public:
    /// constructor
    ToolkitApp();
    /// open the application
    virtual bool Open();
	/// close the application
	virtual void Close();

    /// retrieve nebula build date and version
    const Util::String & GetToolkitVersion() const;

protected:
    /// parse command line arguments
    virtual bool ParseCmdLineArgs();
    /// setup project info object
    virtual bool SetupProjectInfo();
    /// print help text
    virtual void ShowHelp();

    Logger logger;
    ProjectInfo projectInfo;
    Platform::Code platform;
    bool waitForKey;

    Util::String toolkitVersion;
	Ptr<ToolkitUtil::ToolkitConsoleHandler> handler;
};

//------------------------------------------------------------------------------
/**
*/
inline const Util::String &
ToolkitApp::GetToolkitVersion() const
{
    return this->toolkitVersion;
}
} // namespace ToolkitUtil
//------------------------------------------------------------------------------
