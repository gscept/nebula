//------------------------------------------------------------------------------
//  assetexporter.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "assetbatchprocessor.h"
#include "io/ioserver.h"
#include "io/assignregistry.h"
#include "toolkit-common/toolkitconsolehandler.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "toolkit-common/text.h"
#include "io/jsonreader.h"
#include "db/dataset.h"
#include "db/sqlite3/sqlite3factory.h"

#include "assetpackager.h"
#include "assetimporter.h"

using namespace Util;
using namespace IO;
namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::AssetBatchProcessor, 'ASEX', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
AssetBatchProcessor::AssetBatchProcessor() :
    packageMode(All),
    importMode(None)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AssetBatchProcessor::~AssetBatchProcessor()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBatchProcessor::UpdateSource()
{
    this->RecurseValidateIntermediates("intermediate:");
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBatchProcessor::ProcessFile(const IO::URI& file)
{
    IoServer* ioServer = IoServer::Instance();
    Util::String const ext = file.AsString().GetFileExtension();
    Util::String fileName = file.AsString().ExtractFileName();
    fileName.StripFileExtension();

    if ((this->importMode & ImportModes::FBX) && ext == "fbx")
    {
        ToolkitUtil::ImportFBX(file, this->folder, ToolkitUtil::ImportFlags(), 1.0f, this->logger);
    }

    if ((this->importMode & ImportModes::GLTF) && (ext == "gltf" || ext == "glb"))
    {
        ToolkitUtil::ImportGLTF(file, this->folder, ToolkitUtil::ImportFlags(), 1.0f, this->logger);
    }

    if ((this->importMode & ImportModes::Images) && (ext == "tga" || ext == "png" || ext == "jpg" || ext == "jpeg"))
    {
        ToolkitUtil::ImportTexture(file, this->folder);
    }

    if ((this->importMode & ImportModes::Sound) && (ext == "wav" || ext == "ogg" || ext == "mp3"))
    {
        ToolkitUtil::ImportAudio(file, this->folder);
    }

    if ((this->packageMode & PackageModes::Models) && ext == "namdl")
    {
        Util::String dstFile = Util::String::Sprintf("mdl:%s/%s", this->folder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackageModel(file, this->folder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
    else if ((this->packageMode & PackageModes::Meshes) && ext == "namsh")
    {
        Util::String dstFile = Util::String::Sprintf("msh:%s/%s", this->folder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackageMesh(file, this->folder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
    else if ((this->packageMode & PackageModes::Textures) && ext == "natex")
    {
        Util::String dstFile = Util::String::Sprintf("tex:%s/%s", this->folder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackageTexture(file, this->folder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
    else if ((this->packageMode & PackageModes::Materials) && ext == "namat")
    {
        Util::String dstFile = Util::String::Sprintf("mat:%s/%s", this->folder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackageMaterial(file, this->folder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
    else if ((this->packageMode & PackageModes::Particles) && ext == "napar")
    {
        Util::String dstFile = Util::String::Sprintf("par:%s/%s", this->folder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackageParticle(file, this->folder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
    else if ((this->packageMode & PackageModes::Particles) && ext == "naani")
    {
        Util::String dstFile = Util::String::Sprintf("ani:%s/%s", this->folder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackageAnimation(file, this->folder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
    else if ((this->packageMode & PackageModes::Particles) && ext == "naske")
    {
        Util::String dstFile = Util::String::Sprintf("ske:%s/%s", this->folder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackageSkeleton(file, this->folder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
    else if ((this->packageMode & PackageModes::Audio) && ext == "naaud")
    {
        Util::String dstFile = Util::String::Sprintf("audio:%s/%s", this->folder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackageAudio(file, this->folder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
    else if ((this->packageMode & PackageModes::Physics) && ext == "actor")
    {
        Util::String dstFile = Util::String::Sprintf("physics:%s/%s", this->folder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackagePhysics(file, this->folder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBatchProcessor::ProcessDir(const Util::String& category)
{
    String assetPath = String::Sprintf("src:assets/%s/", category.AsCharPtr());
    n_printf("\n----------------- Batching asset directory %s -----------------\n", Text(URI(assetPath).LocalPath()).Color(TextColor::Blue).Style(FontMode::Bold).AsCharPtr());
    IoServer* ioServer = IoServer::Instance();

    IndexT fileIndex;
    ToolLog log(category);
    Ptr<ToolkitUtil::ToolkitConsoleHandler> console = ToolkitUtil::ToolkitConsoleHandler::Instance();
    this->folder = assetPath;

    if (this->importMode & ImportModes::FBX)
    {
        // import FBX files
        this->logger->Print("\nFBXs -----------\n");
        Array<String> files = ioServer->ListFiles(assetPath, "*.fbx");
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to import\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "FBX Import", files[fileIndex]);
            }
        }
    }

    if (this->importMode & ImportModes::GLTF)
    {
        // import GLTF files
        this->logger->Print("\nGLTFs -----------\n");
        Array<String> files = ioServer->ListFiles(assetPath, "*.gltf");
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to import\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "GLTF Import", files[fileIndex]);
            }
        }
    }

    if (this->importMode & ImportModes::Images)
    {
        // import image files
        this->logger->Print("\nImages -----------\n");
        Array<String> files = ioServer->ListFiles(assetPath, "*.png;*.jpg;*.jpeg;*.tga;*.dds;*.exr;*.bmp");
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to import\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "Image Import", files[fileIndex]);
            }
        }
    }

    if (this->importMode & ImportModes::Sound)
    {
        // import sound files
        this->logger->Print("\nSound -----------\n");
        Array<String> files = ioServer->ListFiles(assetPath, "*.wav;*.mp3;*.ogg;");
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to import\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "Sound Import", files[fileIndex]);
            }
        }
    }

    if (this->packageMode & PackageModes::Meshes)
    {
        // export meshes
        this->logger->Print("\nMeshes --------------\n");
        Array<String> files = ioServer->ListFiles(assetPath, "*.namsh");
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to package\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "Model", files[fileIndex]);
            }
        }
    }

    if (this->packageMode & PackageModes::Models)
    {
        // export models
        this->logger->Print("\nModels --------------\n");
        Array<String> files = ioServer->ListFiles(assetPath, "*.namdl");
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to package\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "Model", files[fileIndex]);
            }
        }
    }

    if (this->packageMode & PackageModes::Textures)
    {
        // export textures
        Array<String> files = ioServer->ListFiles(assetPath, "*.natex");

        this->logger->Print("\nTextures ------------\n");
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to package\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "Texture", files[fileIndex]);
            }
        }
    }

    if (this->packageMode & PackageModes::Materials)
    {
        this->logger->Print("\nMaterials ------------\n");
        Array<String> files = ioServer->ListFiles(assetPath, "*.namat");
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to package\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "Material", files[fileIndex]);
            }
        }
    }

    if (this->packageMode & PackageModes::Particles)
    {
        this->logger->Print("\nParticles ------------\n");
        Array<String> files = ioServer->ListFiles(assetPath, "*.napar");
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to package\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "Particle", files[fileIndex]);
            }
        }
    }

    if (this->packageMode & PackageModes::Animations)
    {
        this->logger->Print("\nAnimations ------------\n");
        Array<String> files = ioServer->ListFiles(assetPath, "*.naani");
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to package\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "Particle", files[fileIndex]);
            }
        }
    }

    if (this->packageMode & PackageModes::Skeletons)
    {
        this->logger->Print("\nSkeletons ------------\n");
        Array<String> files = ioServer->ListFiles(assetPath, "*.naskl");
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to package\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "Particle", files[fileIndex]);
            }
        }
    }

    if (this->packageMode & PackageModes::Audio)
    {
        this->logger->Print("\nAudio ---------------\n");
        Array<String> files = ioServer->ListFiles(assetPath, "*.naaud");
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to package\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "Audio", files[fileIndex]);
            }
        }
    }
    if (this->packageMode & PackageModes::Physics)
    {
        this->logger->Print("\nPhysics -------------\n");
        Array<String> files = ioServer->ListFiles(assetPath, "*.actor", true);
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to package\n");
        }
        else
        {
            for (auto const& file : files)
            {
                this->ProcessFile(assetPath + file);
                log.AddEntry(console, "Physics", file);
            }
        }
    }
    this->messages.Append(log);
    this->folder = "";
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBatchProcessor::ProcessAll()
{
    IndexT folderIndex;
    Array<String> folders = IoServer::Instance()->ListDirectories("src:assets/", "*");
    for (folderIndex = 0; folderIndex < folders.Size(); folderIndex++)
    {
        this->ProcessDir(folders[folderIndex]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBatchProcessor::ProcessList(const Util::Array<Util::String>& files)
{
    for (Array<String>::Iterator iter = files.Begin(); iter != files.End(); iter++)
    {
        const Util::String& str = *iter;
        if (IO::IoServer::Instance()->FileExists(str))
        {
            this->ProcessFile(str);
        }
        else if (IO::IoServer::Instance()->DirectoryExists(str))
        {
            Array<String> filesInFolder = IoServer::Instance()->ListFiles(str, "*");
            filesInFolder.AppendArray(IoServer::Instance()->ListDirectories(str, "*"));
            for (auto& f : filesInFolder)
                f = str + "/" + f;
            this->ProcessList(filesInFolder);
        }
    }
}


} // namespace ToolkitUtil
