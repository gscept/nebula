#pragma once
//------------------------------------------------------------------------------
/**
    @class Tools::SetupApplication
    
    Sets up default Nebula paths
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "distributedtools/distributedtoolkitapp.h"

//------------------------------------------------------------------------------
namespace Tools
{
class SetupApplication : public ToolkitUtil::ToolkitApp
{
public:
    /// constructor
    SetupApplication();
    /// destructor
    virtual ~SetupApplication();

    /// opens the application
    bool Open();
    /// runs the application
    void DoWork();
private:
    /// parse command line arguments
    bool ParseCmdLineArgs();
    /// setup project info object
    bool SetupProjectInfo();
    /// print help text
    void ShowHelp();
}; 
} // namespace Tools
//------------------------------------------------------------------------------