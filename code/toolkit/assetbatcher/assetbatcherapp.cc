//------------------------------------------------------------------------------
//  fbxbatcher3app.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "toolkitutil/fbx/nfbxexporter.h"
#include "assetbatcherapp.h"
#include "io/assignregistry.h"
#include "core/coreserver.h"
#include "io/textreader.h"
#include "asset/assetexporter.h"
#include "io/console.h"
#ifdef WIN32
#include "io/win32/win32consolehandler.h"
#else
#include "io/posix/posixconsolehandler.h"
#endif

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
AssetBatcherApp::AssetBatcherApp()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AssetBatcherApp::~AssetBatcherApp()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool 
AssetBatcherApp::Open()
{
    if (DistributedToolkitApp::Open())
    {
		Ptr<IO::Console> console = IO::Console::Instance();		
#ifdef WIN32
		const Util::Array<Ptr<IO::ConsoleHandler>> & handlers = console->GetHandlers();
		for (int i = 0; i < handlers.Size(); i++)
		{
			if (handlers[i]->IsA(Win32::Win32ConsoleHandler::RTTI))
			{
				//console->RemoveHandler(handlers[i]);
			}
		}
#endif		
        this->modelDatabase = ToolkitUtil::ModelDatabase::Create();
        this->modelDatabase->Open();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBatcherApp::Close()
{	
    if (this->modelDatabase.isvalid())
    {
        this->modelDatabase->Close();
        this->modelDatabase = 0;
    }    
    DistributedToolkitApp::Close();
}

//------------------------------------------------------------------------------
/**
*/
void 
AssetBatcherApp::DoWork()
{
    Ptr<AssetExporter> exporter = AssetExporter::Create();
	String dir = "";
	String file = "";
	ExporterBase::ExportFlag exportFlag = ExporterBase::All;
	
	bool force = false;
	if (this->args.HasArg("-dir"))
	{
		exportFlag = ExporterBase::Dir;
		dir = this->args.GetString("-dir");
	}
	if (this->args.HasArg("-force"))
	{
		force = this->args.GetBoolFlag("-force");
	}

	IO::AssignRegistry::Instance()->SetAssign(Assign("home","proj:"));
	exporter->Open();
	exporter->SetForce(force);
	if (force)
	{
		exporter->SetExportMode(AssetExporter::All | AssetExporter::ForceFBX | AssetExporter::ForceModels | AssetExporter::ForceSurfaces);
	}
	exporter->SetExportFlag(exportFlag);
	exporter->SetPlatform(this->platform);
	exporter->SetProgressPrecision(PRECISION);
	if (this->args.HasArg("-system"))
	{
		exporter->ExportSystem();
	}
	else
	{
		if (this->listfileArg.IsValid())
		{
			Array<String> fileList = CreateFileList();
			exporter->ExportList(fileList);
		}
		else
		{
			int files = IO::IoServer::Instance()->ListDirectories("src:assets/", "*").Size();
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
			}
		}
	}
	exporter->Close();

#if 0
	// output to stderr for parsing of tools
	const Util::Array<ToolkitUtil::ToolLog>& failedFiles = exporter->GetMessages();
	
	Ptr<IO::MemoryStream> stream = IO::MemoryStream::Create();
	stream->SetAccessMode(IO::Stream::WriteAccess);
	Ptr<IO::XmlWriter> writer = IO::XmlWriter::Create();
	writer->SetStream(stream.cast<IO::Stream>());
	writer->Open();
	writer->BeginNode("ToolLogs");
	for (auto iter = failedFiles.Begin(); iter != failedFiles.End(); iter++)
	{
		iter->ToString(writer);
	}
	writer->EndNode();
	writer->Close();
	// reopen stream
	stream->Open();
	void * str = stream->Map();
	Util::String streamString;
	streamString.Set((const char*)str, stream->GetSize());		
	fprintf(stderr, "%s", streamString.AsCharPtr());
#endif
	// if we have any errors, set the return code to be errornous	
	if (exporter->HasErrors()) this->SetReturnCode(-1);
}

//------------------------------------------------------------------------------
/**
*/
bool 
AssetBatcherApp::ParseCmdLineArgs()
{
	return DistributedToolkitApp::ParseCmdLineArgs();
}

//------------------------------------------------------------------------------
/**
*/
bool 
AssetBatcherApp::SetupProjectInfo()
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
AssetBatcherApp::ShowHelp()
{
	n_printf("NebulaT asset batcher.\n"
		"(C) 2016 Individual contributors, see AUTHORS file.\n");
	n_printf("-help         --display this help\n"
			 "-force        --ignores time stamps\n"
			 "-dir          --category name\n"
			 "-system       \n"
			 "-platform     --export platform");
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Util::String>
AssetBatcherApp::CreateFileList()
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
            res.Append(category);
		}	

		// update progressbar in batchexporter
		Ptr<Base::ExporterBase> dummy = Base::ExporterBase::Create();
        dummy->SetProgressMinMax(0, res.Size() * PRECISION);
	}	
	return res;
}


} // namespace Toolkit

