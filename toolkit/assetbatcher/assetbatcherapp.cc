//------------------------------------------------------------------------------
//  assetbatcherapp.cc
//  (C) 2012-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "toolkitutil/fbx/nfbxexporter.h"
#include "assetbatcherapp.h"
#include "io/assignregistry.h"
#include "core/coreserver.h"
#include "io/textreader.h"
#include "asset/assetexporter.h"
#include "io/console.h"
#include "profiling/profiling.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "flat/physics/material.h"
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
#if NEBULA_ENABLE_PROFILING
    Profiling::ProfilingRegisterThread();
#endif
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
        Flat::FlatbufferInterface::Init();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBatcherApp::OnBeforeRunLocal()
{
    IO::IoServer::Instance()->CreateDirectory("phys:");
    Flat::FlatbufferInterface::LoadSchema("file:///work:data/flatbuffer/physics/material.fbs");
    IO::URI tablePath = "root:work/data/tables/physicsmaterials.json";
    CompileFlatbuffer(Physics::Materials, tablePath, "phys:");
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
        this->modelDatabase = nullptr;
    }
    DistributedToolkitApp::Close();
}

//------------------------------------------------------------------------------
/**
*/
void 
AssetBatcherApp::DoWork()
{
#if NEBULA_ENABLE_PROFILING
    Profiling::ProfilingNewFrame();
#endif
    Ptr<AssetExporter> exporter = AssetExporter::Create();
    String dir = "";
    String file = "";
    String source = "";
    ExporterBase::ExportFlag exportFlag = ExporterBase::All;
    Dictionary<String, String> sources;

    // override dests with settings from projectinfo
    AssignRegistry::Instance()->SetAssign(Assign("tex", this->projectInfo.GetAttr("TextureDestDir")));

    bool force = false;
    if (this->args.HasArg("-source"))
    {
        source = this->args.GetString("-source");
        if (!sources.Contains(source))
        {
            this->logger.Error("Unknown source: %s\n", source.AsCharPtr());
            return;
        }
    }
    if (this->args.HasArg("-work"))
    {
        String workOverride = this->args.GetString("-work");
        workOverride = IO::IoServer::NativePath(workOverride);
        sources.Add("work", workOverride);
    }
    else
    {
        sources = this->projectInfo.GetListAttr("AssetSources");
    }
    if (this->args.HasArg("-asset"))
    {
        if (source.IsEmpty())
        {
            this->logger.Error("Specified asset argument without source argument\n");
            return;
        }
        exportFlag = ExporterBase::Dir;
        dir = this->args.GetString("-asset");
    }
    if (this->args.HasArg("-force"))
    {
        force = this->args.GetBoolFlag("-force");
    }

    AssignRegistry::Instance()->SetAssign(Assign("home","proj:"));
    exporter->Open();
    exporter->SetForce(force);
    if (force)
    {
        exporter->SetExportMode(AssetExporter::All | AssetExporter::ForceFBX | AssetExporter::ForceModels | AssetExporter::ForceSurfaces | AssetExporter::ForceGLTF);
    }
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
        
        switch (exportFlag)
        {
            case ExporterBase::All:
            {
                for (auto const& src : sources)
                {
                    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("src", src.Value()));
                    int files = IO::IoServer::Instance()->ListDirectories("src:assets/", "*").Size();
                    exporter->UpdateSource();
                    exporter->SetProgressMinMax(0, files * PRECISION);
                    exporter->ExportAll();
                }
            }
            break;
            case ExporterBase::Dir:
            {
                IO::AssignRegistry::Instance()->SetAssign(IO::Assign("src", sources[source]));
                int files = IO::IoServer::Instance()->ListDirectories("src:assets/", "*").Size();
                exporter->UpdateSource();
                exporter->SetProgressMinMax(0, files * PRECISION);
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
    n_printf("Nebula asset batcher.\n"
        "(C) 2012-2020 Individual contributors, see AUTHORS file.\n");
    n_printf("-help         --display this help\n"
             "-force        --ignores time stamps\n"
             "-asset        --asset name, implies source argument\n"
             "-source       --select asset source from projectinfo, default all\n"
             "-work         --batch a non-registered work folder into the project\n"
             "-project      --projectinfo override\n");
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Util::String>
AssetBatcherApp::CreateFileList()
{
    this->logger.Warning("Running parallel builds will currently only build proj:");
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
        reader = nullptr;
        readStream = nullptr;
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

