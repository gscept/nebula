//------------------------------------------------------------------------------
//  setupapplication.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "setupapplication.h"
#include "system/nebulasettings.h"

using namespace System;
using namespace ToolkitUtil;

namespace Tools
{
//------------------------------------------------------------------------------
/**
*/
SetupApplication::SetupApplication()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
SetupApplication::~SetupApplication()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
bool 
SetupApplication::Open()
{
	bool retval = ToolkitApp::Open();
	this->DoWork();
	return retval;
}

//------------------------------------------------------------------------------
/**
*/
void 
SetupApplication::DoWork()
{
	switch (this->platform)
	{
	case Platform::Win32:
    case Platform::Linux:
		{
			NebulaSettings::WriteString("gscept","ToolkitShared", "path", IO::URI("root:").GetHostAndLocalPath());
			NebulaSettings::WriteString("gscept","ToolkitShared", "workdir", IO::URI("root:").GetHostAndLocalPath());			
			break;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
bool 
SetupApplication::ParseCmdLineArgs()
{
	return ToolkitApp::ParseCmdLineArgs();
}

//------------------------------------------------------------------------------
/**
*/
bool 
SetupApplication::SetupProjectInfo()
{
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void 
SetupApplication::ShowHelp()
{
	n_printf("NebulaT Setup application.\n"
		"(C) 2012-2016 Individual contributors, see AUTHORS file.\n");
	n_printf("-help         --display this help\n"
		"-nody      --nody path override"
		"-working   --nebula working folder (where bins are located)\n"
		"-project   --nebula project trunk (if empty, attempts to use registry)\n"
		"-platform     --export platform");
}
} // namespace Tools