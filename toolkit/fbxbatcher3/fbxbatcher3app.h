#pragma once
//------------------------------------------------------------------------------
/**
    @class TookitUtil::FBXBatcher3App
    
    Entry point for FBX batcher
    
    (C) 2012-2020 Individual contributors, see AUTHORS file
*/
#include "toolkit-common/distributedtools/distributedtoolkitapp.h"
#include "modelutil/modeldatabase.h"

//------------------------------------------------------------------------------
namespace Toolkit
{
class FBXBatcher3App : public DistributedTools::DistributedToolkitApp
{

public:

    /// opens the application
    bool Open();
    /// runs the application
    void DoWork();
    /// constructor
    FBXBatcher3App();
    /// destructor
    virtual ~FBXBatcher3App();
protected:
    /// creates file list for job-driven exporting
    virtual Util::Array<Util::String> CreateFileList();
private:
    /// parse command line arguments
    bool ParseCmdLineArgs();
    /// setup project info object
    bool SetupProjectInfo();
    /// print help text
    void ShowHelp();

    Ptr<ToolkitUtil::ModelDatabase> modelDatabase;
}; 
} // namespace TookitUtil
//------------------------------------------------------------------------------