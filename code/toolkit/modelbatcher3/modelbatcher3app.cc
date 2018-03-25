//------------------------------------------------------------------------------
//  modelbatcher3app.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "modelbatcher3app.h"
#include "io/ioserver.h"
#include "io/textreader.h"

using namespace Util;
using namespace ToolkitUtil;
using namespace IO;

namespace Toolkit
{

//------------------------------------------------------------------------------
/**
*/
ModelBuilder3App::ModelBuilder3App()
{
	// empty	
}

//------------------------------------------------------------------------------
/**
*/
ModelBuilder3App::~ModelBuilder3App()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
bool 
ModelBuilder3App::Open()
{	
	if (DistributedToolkitApp::Open())
	{
		return true;
	}
	
	return false;
}


//------------------------------------------------------------------------------
/**
*/
void 
ModelBuilder3App::DoWork()
{
	String dir = "";
	String file = "";
	String projectFolder = "proj:";

	if (this->args.HasArg("-projectdir"))
	{
		projectFolder = this->args.GetString("-projectdir") + "/";
	}	 
    IO::AssignRegistry::Instance()->SetAssign(Assign("home", "proj:"));

	Array<String> fileList = CreateFileList();

	// setup objects
	this->builder = ModelBuilder::Create();
	this->database = ModelDatabase::Create();

	// open database
	this->database->Open();

	// export all files
	IndexT i;
	for (i = 0; i < fileList.Size(); i++)
	{
		// get file
		String file = fileList[i];
		file.StripFileExtension();

		// get constants, attributes and physics
		Ptr<ModelConstants> constants = ModelDatabase::Instance()->LookupConstants(file);
		Ptr<ModelAttributes> attributes = ModelDatabase::Instance()->LookupAttributes(file);
		Ptr<ModelPhysics> physics = ModelDatabase::Instance()->LookupPhysics(file);

		// setup model builder
		this->builder->SetAttributes(attributes);
		this->builder->SetConstants(constants);
		this->builder->SetPhysics(physics);

		// create model path
		String modelPath;
		modelPath.Format("mdl:%s.n3", file.AsCharPtr());

		// create physics path
		String physicsPath;
		physicsPath.Format("phys:%s.np3", file.AsCharPtr());
		
		// export
		this->builder->SaveN3(modelPath, this->platform);
		this->builder->SaveN3Physics(physicsPath, this->platform);
		n_printf("Building model: %s\n", file.AsCharPtr());
		n_printf("    Result: \n        [graphics = '%s'] \n        [physics = '%s']\n\n", modelPath.AsCharPtr(), physicsPath.AsCharPtr());
	}

	// kill database
	this->database->Close();

	this->database = 0;
	this->builder = 0;
}

//------------------------------------------------------------------------------
/**
*/
bool 
ModelBuilder3App::ParseCmdLineArgs()
{
	return DistributedToolkitApp::ParseCmdLineArgs();	
}

//------------------------------------------------------------------------------
/**
*/
bool 
ModelBuilder3App::SetupProjectInfo()
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
ModelBuilder3App::ShowHelp()
{
	n_printf("Nebula Trifid model batcher.\n"
			"(C) Gustav Sterbrant 2011.\n"
			"-help      -- display this help\n"
			"-platform  -- select platform (win32)\n");
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Util::String> 
ModelBuilder3App::CreateFileList()
{
	Util::Array<Util::String> res;

	// create list from given filelist
	if(this->listfileArg.IsValid())
	{
		// create path to list of arguments
		URI listUri(this->listfileArg);

		// open stream and reader
		Ptr<Stream> readStream = IoServer::Instance()->CreateStream(listUri);
		readStream->SetAccessMode(Stream::ReadAccess);

		// create reader
		Ptr<TextReader> reader = TextReader::Create();
		reader->SetStream(readStream);

		// open stream
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
			Array<String> files = IoServer::Instance()->ListFiles(category, "*.constants");
			for (int fileIndex = 0; fileIndex < files.Size(); fileIndex++)
			{
				String file = directories[directoryIndex] + "/" + files[fileIndex];
				res.Append(file);			
			}
		}	
	}	
	return res;
}
} // namespace ToolkitUtil