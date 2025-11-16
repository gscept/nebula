//------------------------------------------------------------------------------
//  assetbatcherapp.cc
//  (C) 2012-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"

#include "toolkitutil/model/import/fbx/nfbxexporter.h"

#include "assetbatcherapp.h"
#include "io/assignregistry.h"
#include "core/coreserver.h"
#include "io/textreader.h"
#include "asset/assetexporter.h"
#include "io/console.h"
#include "profiling/profiling.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "flat/physics/material.h"
#include "jobs2/jobs2.h"
#include "system/nebulasettings.h"
#include "toolkit-common/text.h"

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
    IO::IoServer* ioServer = IO::IoServer::Instance();
    ioServer->CreateDirectory("phys:");
    const String flatBufferRoot = "tool:syswork/data/flatbuffer/";
    auto printname = [](const String& name, const String& folder)
    {
        const IO::URI root("tool:syswork/data/flatbuffer/");
        const String rootPath = root.LocalPath();
        n_assert(0 == folder.FindStringIndex(rootPath));
        String base = folder.ExtractToEnd(rootPath.Length()+1);
        String source = String::Sprintf("%s/%s", folder.AsCharPtr(), name.AsCharPtr());
        String target = String::Sprintf("bfbs:%s/%s", base.AsCharPtr(), name.AsCharPtr());
        target.ChangeFileExtension("bfbs");
        Flat::FlatbufferInterface::CompileSchema(source, target);
    };
    ioServer->IterateFolders("tool:syswork/data/flatbuffer/", "*.fbs", printname);
    IO::URI tablePath = "tool:syswork/data/tables/physicsmaterials.json";
    CompileFlatbuffer(Physics::Materials, tablePath, "phys:");
    CompileFlatbuffer(Physics::Materials, "proj:work/data/tables/physicsmaterials.json","phys:");
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
AssetBatcherApp::Run()
{
    Timing::Timer totalTime;
    totalTime.Start();

    DistributedTools::DistributedToolkitApp::Run();

    totalTime.Stop();
    if (this->IsMaster() || !this->listfileArg.IsValid())
    {
        this->logger.Print("\n");
        this->logger.Print("--- Batching took ");
        int minutes = Math::floor(totalTime.GetTime() / 60.0f);
        float seconds = Math::fmod(totalTime.GetTime(), 60.0f);
        Util::String time = Util::String::FromInt(minutes) + "m " + Util::String::FromFloat(seconds) + "s\n";
        this->logger.Print(
            ToolkitUtil::Text(time).Color(ToolkitUtil::TextColor::Green).Style(ToolkitUtil::FontMode::Bold).AsCharPtr()
        );
    }
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

    Jobs2::JobSystemInitInfo systemInit;
    systemInit.name = "JobSystem";
    systemInit.numThreads = 8;
    systemInit.scratchMemorySize = 16_MB;
    systemInit.affinity = System::Cpu::All;
    systemInit.enableIo = true;
    Jobs2::JobSystemInit(systemInit);

    // override dests with settings from projectinfo
    AssignRegistry::Instance()->SetAssign(Assign("tex", this->projectInfo.GetAttr("TextureDestDir")));
    AssignRegistry::Instance()->SetAssign(Assign("export", this->projectInfo.GetAttr("DestDir")));
    AssignRegistry::Instance()->SetAssign(Assign("intermediate", this->projectInfo.GetAttr("IntermediateDir")));

    bool force = false;
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
    if (this->args.HasArg("-source"))
    {
        source = this->args.GetString("-source");
        if (!sources.Contains(source))
        {
            this->logger.Error("Unknown source: %s\n", source.AsCharPtr());
            return;
        }
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

    Flat::FlatbufferInterface::Init();
    AssetExporter::ExportModes mode = AssetExporter::All;
    if (this->args.HasArg("-mode"))
    {
        mode = (AssetExporter::ExportModes)0;
        Util::String exportMode = this->args.GetString("-mode");
        Util::Array<Util::String> modeFlags = exportMode.Tokenize(",");
        if (modeFlags.Find("fbx")) mode |= AssetExporter::FBX;
        if (modeFlags.Find("model")) mode |= AssetExporter::Models;
        if (modeFlags.Find("surface")) mode |= AssetExporter::Surfaces;
        if (modeFlags.Find("texture")) mode |= AssetExporter::Textures;
        if (modeFlags.Find("physics")) mode |= AssetExporter::Physics;
        if (modeFlags.Find("gltf")) mode |= AssetExporter::GLTF;
        if (modeFlags.Find("audio")) mode |= AssetExporter::Audio;
    }

    AssignRegistry::Instance()->SetAssign(Assign("home","proj:"));

    // check to see if the assetbatcher has been updated, in that case, force all assets to be rebuilt.
    IO::FileTime cmdBinModifiedTime = IO::IoServer::Instance()->GetFileWriteTime(this->args.GetCmdName());
    if (System::NebulaSettings::Exists("gscept", "ToolkitShared", "AssetBatcherTimeStamp"))
    {
        Util::String oldFileTime = System::NebulaSettings::ReadString("gscept", "ToolkitShared", "AssetBatcherTimeStamp");
        if (IO::FileTime(oldFileTime) < cmdBinModifiedTime)
        {
            System::NebulaSettings::WriteString("gscept", "ToolkitShared", "AssetBatcherTimeStamp", cmdBinModifiedTime.AsString());
            force = true;
        }
    }
    else
    {
        System::NebulaSettings::WriteString("gscept", "ToolkitShared", "AssetBatcherTimeStamp", cmdBinModifiedTime.AsString());
        force = true;
    }

    exporter->Open();
    exporter->SetExportMode(mode);
    exporter->SetForce(force);
    exporter->SetLogger(&this->logger);
    if (force)
    {
        exporter->SetExportMode(AssetExporter::All | AssetExporter::ForceFBX | AssetExporter::ForceModels | AssetExporter::ForceSurfaces | AssetExporter::ForceGLTF | AssetExporter::ForceAudio);
    }
    exporter->SetExportFlag(exportFlag);
    exporter->SetPlatform(this->platform);
    exporter->SetProgressPrecision(PRECISION);
    
    if (this->listfileArg.IsValid())
    {
        Array<String> fileList = CreateFileList();
        IO::AssignRegistry::Instance()->SetAssign(IO::Assign("src", "proj:work"));
        exporter->UpdateSource();
        exporter->SetProgressMinMax(0, fileList.Size() * PRECISION);
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
            }
            break;
            case ExporterBase::File:
            n_error("Wrong export type, file");
            break;
        }
    }
    
    exporter->Close();

    Jobs2::JobSystemUninit();

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
        "(C) 2012-2024 Individual contributors, see AUTHORS file.\n");
    n_printf("-force       -- ignores time stamps\n"
             "-asset       -- asset name, implies source argument\n"
             "-source      -- select asset source from projectinfo, default all\n"
             "-work        -- batch a non-registered work folder into the project\n"
             "-mode        -- batch only a type of resource, can be: fbx, model, surface, texture, physics, gltf, audio\n"
             "-project     -- projectinfo override\n"
    );

    String additionalHelp = GetArgumentDescriptionString();
    n_printf(additionalHelp.AsCharPtr());
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
            
            Array<String> filesInFolder = IoServer::Instance()->ListFiles(category, "*");
            filesInFolder.AppendArray(IoServer::Instance()->ListDirectories(category, "*"));
            for (auto& f : filesInFolder)
            {
                f = category + "/" + f;
                res.Append(f);
            }
        }

        // update progressbar in batchexporter
        Ptr<Base::ExporterBase> dummy = Base::ExporterBase::Create();
        dummy->SetProgressMinMax(0, res.Size() * PRECISION);
    }
    return res;
}


} // namespace Toolkit

