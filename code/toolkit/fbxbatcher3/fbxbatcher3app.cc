//------------------------------------------------------------------------------
//  fbxbatcher3app.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "toolkitutil/fbx/nfbxexporter.h"
#include "fbxbatcher3app.h"
#include "io/assignregistry.h"
#include "core/coreserver.h"
#include "io/textreader.h"

#define PRECISION 1000000

using namespace IO;
using namespace Util;
using namespace ToolkitUtil;
using namespace Base;

namespace Toolkit
{
//------------------------------------------------------------------------------
/**
*/
FBXBatcher3App::FBXBatcher3App()
{
	this->modelDatabase = ModelDatabase::Create();
	this->modelDatabase->Open();
}

//------------------------------------------------------------------------------
/**
*/
FBXBatcher3App::~FBXBatcher3App()
{
	this->modelDatabase->Close();
	this->modelDatabase = 0;
}

//------------------------------------------------------------------------------
/**
*/
bool 
FBXBatcher3App::Open()
{
	return DistributedToolkitApp::Open();
}

//------------------------------------------------------------------------------
/**
*/
void 
FBXBatcher3App::DoWork()
{
    Ptr<ToolkitUtil::NFbxExporter> exporter = ToolkitUtil::NFbxExporter::Create();
	String dir = "";
	String file = "";
	String projectFolder = "proj:";
	ExporterBase::ExportFlag exportFlag = ExporterBase::All;
	
	bool force = false;
	if (this->args.HasArg("-dir"))
	{
		exportFlag = ExporterBase::Dir;
		dir = this->args.GetString("-dir");
	}
	if (this->args.HasArg("-file"))
	{
		exportFlag = ExporterBase::File;
		file = this->args.GetString("-file");
	}
	if (this->args.HasArg("-projectdir"))
	{
		projectFolder = this->args.GetString("-projectdir") + "/";
	}	 
	if (this->args.HasArg("-force"))
	{
		force = this->args.GetBoolFlag("-force");
	}

    IO::AssignRegistry::Instance()->SetAssign(Assign("home", "proj:"));
	exporter->Open();
	exporter->SetForce(force);
	exporter->SetExportFlag(exportFlag);
	exporter->SetPlatform(this->platform);
	exporter->SetProgressPrecision(PRECISION);

	if (this->listfileArg.IsValid())
	{
		Array<String> fileList = CreateFileList();		
		exporter->ExportList(fileList);
	}
	else
	{
		int files = exporter->CountExports(projectFolder + "work/assets/" + dir, "fbx");
		switch (exportFlag)
		{
		case ExporterBase::All:
			exporter->SetProgressMinMax(0, files*PRECISION);
			exporter->ExportAll();
			break;
		case ExporterBase::Dir:
			exporter->SetProgressMinMax(0, files*PRECISION);
			exporter->ExportDir(dir);
			break;
		case ExporterBase::File:
			exporter->SetCategory(dir);
			exporter->SetFile(file);
			exporter->SetProgressMinMax(0, PRECISION);
			exporter->ExportFile(projectFolder + "work/assets/" + dir + "/" + file);
			break;	
		}
	}	
	exporter->Close();

	// if we have any errors, set the return code to be erroneous
	if (exporter->HasErrors()) this->SetReturnCode(-1);
}

//------------------------------------------------------------------------------
/**
*/
bool 
FBXBatcher3App::ParseCmdLineArgs()
{
	return DistributedToolkitApp::ParseCmdLineArgs();
}

//------------------------------------------------------------------------------
/**
*/
bool 
FBXBatcher3App::SetupProjectInfo()
{
	if (DistributedToolkitApp::SetupProjectInfo())
	{
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
void 
FBXBatcher3App::ShowHelp()
{
	n_printf("NebulaT FBX batcher.\n"
		"(C) 2012-2016 Individual contributors, see AUTHORS file.\n");
	n_printf("-help         --display this help\n"
			 "-force        --ignores time stamps\n"
			 "-dir          --category name\n"
			 "-file         --file name (if empty, dir will be parsed)\n"
			 "-projectdir   --nebula project trunk (if empty, attempts to use registry)\n"
			 "-platform     --export platform");
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Util::String>
FBXBatcher3App::CreateFileList()
{
	Util::Array<Util::String> res;
	// create list from given filelist
    if (this->listfileArg.IsValid())
	{
		URI listUri(this->listfileArg);
		// open stream and reader
		Ptr<Stream> readStream = IoServer::Instance()->CreateStream(listUri);
		readStream->SetAccessMode(Stream::ReadAccess);
		Ptr<TextReader> reader = TextReader::Create();
		reader->SetStream(readStream);
		if (reader->Open())
		{
			// read each line and append to list
			while(!reader->Eof())
			{
				String srcPath = reader->ReadLine();
				srcPath.Trim(" \r\n");
				res.Append(srcPath);				
			}
			// close stream and reader
			reader->Close();
		}
		reader = 0;
		readStream = 0;
	}
	else
	{				
		String workDir = "proj:work/assets";
		Array<String> directories = IoServer::Instance()->ListDirectories(workDir, "*");
		for (int directoryIndex = 0; directoryIndex < directories.Size(); directoryIndex++)
		{
			String category = workDir + "/" + directories[directoryIndex];			
			Array<String> files = IoServer::Instance()->ListFiles(category, "*.fbx");
			for (int fileIndex = 0; fileIndex < files.Size(); fileIndex++)
			{
				String file = directories[directoryIndex] + "/" + files[fileIndex];
				res.Append(file);			
			}
		}	
		// update progressbar in batchexporter
		Ptr<Base::ExporterBase> dummy = Base::ExporterBase::Create();
		dummy->SetProgressMinMax(0,res.Size()*PRECISION);
	}	
	return res;
}


} // namespace Toolkit

