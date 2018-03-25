//------------------------------------------------------------------------------
//  audiobatcherapp.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "audiobatcherapp.h"

namespace Toolkit
{

//------------------------------------------------------------------------------
/**
*/
bool
AudioBatcherApp::ParseCmdLineArgs()
{
    if (ToolkitApp::ParseCmdLineArgs())
    {
        this->audioExporter.SetForceFlag(this->args.GetBoolFlag("-force"));
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
AudioBatcherApp::SetupProjectInfo()
{
    if (ToolkitApp::SetupProjectInfo())
    {
        this->audioExporter.SetPlatform(this->platform);
        if (this->projectInfo.HasAttr("AudioTool"))
        {
            this->audioExporter.SetToolPath(this->projectInfo.GetPathAttr("AudioTool"));
        }
		if (this->projectInfo.HasAttr("AudioProjectFile"))
		{
			this->audioExporter.SetProjectFile(this->projectInfo.GetAttr("AudioProjectFile"));
		}
		if (this->projectInfo.HasAttr("AudioSrcDir"))
		{
			this->audioExporter.SetSrcDir(this->projectInfo.GetAttr("AudioSrcDir"));
		}
		this->audioExporter.SetDstDir(this->projectInfo.GetAttr("AudioDstDir"));
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
AudioBatcherApp::ShowHelp()
{
    n_printf("NebulaT audio batch compiler.\n"
             "(C) Individual Authors, see AUTHORS file.\n"
             "-help       -- display this help\n"
             "-platform   -- select platform (win32, xbox360, wii, ps3)\n"
             "-fmod       -- use fmod as audio-engine\n"
             "-waitforkey -- wait for key when complete\n"
             "-force      -- force rebuild\n");
}

//------------------------------------------------------------------------------
/**
*/
void
AudioBatcherApp::Run()
{
    // parse command line args
    if (!this->ParseCmdLineArgs())
    {
        return;
    }

    // setup the project info object
    if (!this->SetupProjectInfo())
    {
        return;
    }

    // perform export process
    if (!this->audioExporter.Export())
    {
        this->SetReturnCode(-1);
        return;
    }
}

} // namespace Toolkit
