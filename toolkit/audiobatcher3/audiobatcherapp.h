#pragma once
//------------------------------------------------------------------------------
/**
    @class Toolkit::AudioBatcherApp
    
    Application class for audiobatcher3 tool.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "toolkitutil/toolkitapp.h"
#include "toolkitutil/audioexporter.h"

//------------------------------------------------------------------------------
namespace Toolkit
{
class AudioBatcherApp : public ToolkitUtil::ToolkitApp
{
public:
    /// run the application
    virtual void Run();

private:
    /// parse command line arguments
    virtual bool ParseCmdLineArgs();
    /// setup project info object
    virtual bool SetupProjectInfo();
    /// print help text
    virtual void ShowHelp();

    ToolkitUtil::AudioExporter audioExporter;
};

} // namespace Toolkit
//------------------------------------------------------------------------------
    