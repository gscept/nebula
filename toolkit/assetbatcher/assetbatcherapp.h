#pragma once
//------------------------------------------------------------------------------
/**
    @class TookitUtil::AssetBatcherApp
    
    Entry point for asset batcher
    
    (C) 2012-2020 Individual contributors, see AUTHORS file
*/
#include "toolkit-common/distributedtools/distributedtoolkitapp.h"
#include "modelutil/modeldatabase.h"
#include "toolkit-common/toolkitconsolehandler.h"

//------------------------------------------------------------------------------
namespace Toolkit
{
class AssetBatcherApp : public DistributedTools::DistributedToolkitApp
{

public:
    /// constructor
    AssetBatcherApp();
    /// destructor
    virtual ~AssetBatcherApp();

    /// opens the application
    bool Open();
    /// close the application
    void Close();
    /// runs the application
    void DoWork();
protected:
    /// creates file list for job-driven exporting
    virtual Util::Array<Util::String> CreateFileList();
    ///
    virtual void OnBeforeRunLocal();

private:
    /// parse command line arguments
    bool ParseCmdLineArgs();
    /// setup project info object
    bool SetupProjectInfo();
    /// print help text
    void ShowHelp();

    Ptr<ToolkitUtil::ModelDatabase> modelDatabase;
    Ptr<ToolkitUtil::ToolkitConsoleHandler> handler;
}; 
} // namespace TookitUtil
//------------------------------------------------------------------------------