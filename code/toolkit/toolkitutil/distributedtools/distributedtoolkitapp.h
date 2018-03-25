#pragma once
//------------------------------------------------------------------------------
/**
    @class DistributedTools::DistributedToolkitApp

    Base class for distributed tools. It contains a "master" part and
    a "slave" part.
    
    The master part is implemented in this class and
    provides the functionality to distribute the tasks of this tool.

    The slave part should be overwritten by subclasses and provides functions
    to do the main work of the tool. This is the DoWork() method only at the moment.

    In subclasses you have to use the predefined assigns as often as possible instead
    of the attributes from the projectinfo.xml. The assigns may vary based on
    the mode the tool is currently running in.

    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "toolkitutil/toolkitapp.h"
#include "distributedtools/shareddircontrol.h"
#include "distributedtools/shareddircreator.h"
#include "distributedtools/distributedjobs/distributedjob.h"

//------------------------------------------------------------------------------
namespace DistributedTools
{
class DistributedToolkitApp : public ToolkitUtil::ToolkitApp
{
public:
    /// constructor
    DistributedToolkitApp();
    /// constructor
    virtual ~DistributedToolkitApp();

    /// parse command line arguments
    virtual bool ParseCmdLineArgs();
    /// get command line argument description
    virtual Util::String GetArgumentDescriptionString();
    /// open the application
    virtual bool Open();
    /// run the application in master or slave mode depending on the input parameters
    virtual void Run();
    /// close the application
    virtual void Close();

    /// get SharedDirControl
    const Ptr<SharedDirControl> & GetSharedDirControl();
    /// determine if application is distributed
    bool IsDistributed();
    /// determine if application is running in slave mode
    bool IsSlave();

protected:
    /// split task of this application into jobs and process them
    virtual void CreateAndProcessJobs();
    /// create a string list of all files that are processed by this application
    virtual Util::Array<Util::String> CreateFileList();
    /// checks svn status of project dir and gets current revision
    virtual Util::String GetCurrentRevision();
    /// wait for user input
    virtual void WaitForKey();
    /// do the work of the application (override in subclasses)
    virtual void DoWork();
    /// actions that should be done at local machine before any work
    virtual void OnBeforeRunLocal();
    /// actions that should be done at local machine after any work
    virtual void OnAfterRunLocal();
    /// Generate a list of jobs that are processed at the start of every parallel job group
    virtual Util::Array<Ptr<DistributedJob>> GenerateInitializeJobList();
    /// Generate a list of jobs that are processed after a parallel job group has finished
    virtual Util::Array<Ptr<DistributedJob>> GenerateFinalizeJobList();

    /// things that every slave should do before execution, unless it is not distributed
    virtual void OnBeforeRunDistSlave();
    /// things that every slave should do after execution, unless it is not distributed
    virtual void OnAfterRunDistSlave();
    /// things that only master should do before execution, unless it is not distributed
    virtual void OnBeforeRunDistMaster();
    /// things that only master should do after execution, unless it is not distributed
    virtual void OnAfterRunDistMaster();

    Util::String projectdirArg;
    Util::String dirArg;
    Util::String fileArg;
    Util::String listfileArg;
    bool forceArg;
    bool verboseFlag;

private: 
    /// analyze the command line arguments and return additional arguments
    Util::String GetAdditionalArguments();
    /// removes all content of given directory recursively
    void DeleteDirectoryContent(const IO::URI & path);

    Ptr<SharedDirControl> sharedDirControl;
    Ptr<SharedDirCreator> sharedDirCreator;
    Util::String shareddirArg;
    bool slaveArg;
    bool masterMode;
    bool distributedArg;
    int jobsArg;
    int processorsArg;      
};

//------------------------------------------------------------------------------
/**
    determine if application is distributed	
*/
inline
bool
DistributedToolkitApp::IsDistributed()
{
    return this->distributedArg;
}

//------------------------------------------------------------------------------
/**
    determine if application is in slave mode	
*/
inline
bool
DistributedToolkitApp::IsSlave()
{
    return this->slaveArg;
}

//------------------------------------------------------------------------------
/**
    determine if application is distributed	
*/
inline
const Ptr<SharedDirControl> &
DistributedToolkitApp::GetSharedDirControl()
{
    return this->sharedDirControl;
}

} // namespace DistributedTools