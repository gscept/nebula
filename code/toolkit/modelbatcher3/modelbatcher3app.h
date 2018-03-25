#pragma once
//------------------------------------------------------------------------------
/**
    @class Toolkitutil::ModelBuilder3App
    
	Builds models for all .constants, .attributes and .physics files
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "distributedtools/distributedtoolkitapp.h"
#include "toolkitutil/modelutil/modelbuilder.h"
#include "modelutil/modeldatabase.h"

//------------------------------------------------------------------------------
namespace Toolkit
{
class ModelBuilder3App : public DistributedTools::DistributedToolkitApp
{

public:
	/// constructor
	ModelBuilder3App();
	/// destructor
	virtual ~ModelBuilder3App();

	/// runs the application
	void DoWork();
	/// opens application
	bool Open();

protected:
	/// creates file list for job-driven exporting
	virtual Util::Array<Util::String> CreateFileList();
private:
	/// converts all files from source and puts them in destination
	void Convert();
	/// parse command line arguments
	virtual bool ParseCmdLineArgs();
	/// setup project info object
	virtual bool SetupProjectInfo();
	/// print help text
	virtual void ShowHelp();

	Ptr<ToolkitUtil::ModelDatabase> database;
	Ptr<ToolkitUtil::ModelBuilder> builder;
}; 
} // namespace Toolkitutil
//------------------------------------------------------------------------------