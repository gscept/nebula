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

    Util::String packageFolder = this->folder.StripSubstring("src:assets/");
    if ((this->packageMode & PackageModes::Models) && ext == "namdl")
    {
        Util::String dstFolder = Util::String::Sprintf("mdl:%s", packageFolder.AsCharPtr());
        Util::String dstFile = Util::String::Sprintf("%s%s", dstFolder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackageModel(file, dstFolder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
    else if ((this->packageMode & PackageModes::Meshes) && ext == "namsh")
    {
        Util::String dstFolder = Util::String::Sprintf("msh:%s", packageFolder.AsCharPtr());
        Util::String dstFile = Util::String::Sprintf("%s%s", dstFolder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackageMesh(file, dstFolder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
    else if ((this->packageMode & PackageModes::Textures) && ext == "natex")
    {
        Util::String dstFolder = Util::String::Sprintf("tex:%s", packageFolder.AsCharPtr());
        Util::String dstFile = Util::String::Sprintf("%s%s", dstFolder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackageTexture(file, dstFolder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
    else if ((this->packageMode & PackageModes::Materials) && ext == "namat")
    {
        Util::String dstFolder = Util::String::Sprintf("mat:%s", packageFolder.AsCharPtr());
        Util::String dstFile = Util::String::Sprintf("%s%s", dstFolder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackageMaterial(file, dstFolder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
    else if ((this->packageMode & PackageModes::Particles) && ext == "napar")
    {
        Util::String dstFolder = Util::String::Sprintf("par:%s", packageFolder.AsCharPtr());
        Util::String dstFile = Util::String::Sprintf("%s%s", dstFolder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackageParticle(file, dstFolder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
    else if ((this->packageMode & PackageModes::Particles) && ext == "naani")
    {
        Util::String dstFolder = Util::String::Sprintf("ani:%s", packageFolder.AsCharPtr());
        Util::String dstFile = Util::String::Sprintf("%s%s", dstFolder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackageAnimation(file, dstFolder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
    else if ((this->packageMode & PackageModes::Particles) && ext == "naske")
    {
        Util::String dstFolder = Util::String::Sprintf("ske:%s", packageFolder.AsCharPtr());
        Util::String dstFile = Util::String::Sprintf("%s%s", dstFolder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackageSkeleton(file, dstFolder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
    else if ((this->packageMode & PackageModes::Audio) && ext == "naaud")
    {
        Util::String dstFolder = Util::String::Sprintf("audio:%s", packageFolder.AsCharPtr());
        Util::String dstFile = Util::String::Sprintf("%s%s", dstFolder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackageAudio(file, dstFolder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
    else if ((this->packageMode & PackageModes::Physics) && ext == "actor")
    {
        Util::String dstFolder = Util::String::Sprintf("phys:%s", packageFolder.AsCharPtr());
        Util::String dstFile = Util::String::Sprintf("%s%s", dstFolder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackagePhysics(file, dstFolder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBatchProcessor::ProcessDir(const Util::String& dir)
{
    IoServer* ioServer = IoServer::Instance();

    // Scan depth first
    Array<String> folders = ioServer->ListDirectories(dir, "*");
    IndexT folderIndex;
    for (folderIndex = 0; folderIndex < folders.Size(); folderIndex++)
    {
        this->ProcessDir(Util::Format("%s%s/", dir.AsCharPtr(), folders[folderIndex].AsCharPtr()));
    }

    n_printf("\n----------------- Batching asset directory %s -----------------\n", Text(URI(dir).LocalPath()).Color(TextColor::Blue).Style(FontMode::Bold).AsCharPtr());

    IndexT fileIndex;
    ToolLog log(dir);
    Ptr<ToolkitUtil::ToolkitConsoleHandler> console = ToolkitUtil::ToolkitConsoleHandler::Instance();
    this->folder = dir;

    Array<String> files = ioServer->ListFiles(dir, "*");

    Array<String> fbxFiles, gltfFiles, imageFiles, soundFiles, meshFiles, modelFiles, textureFiles, materialFiles, particleFiles, animationFiles, skeletonFiles, audioFiles, physicsFiles;
    for (const auto& file : files)
    {
        if (file.EndsWithString(".fbx"))
        {
            fbxFiles.Append(file);
        }
        else if (file.EndsWithString(".gltf") || file.EndsWithString(".glb"))
        {
            gltfFiles.Append(file);
        }
        else if (file.EndsWithString(".png") || file.EndsWithString(".jpg") || file.EndsWithString(".jpeg") || file.EndsWithString(".tga") || file.EndsWithString(".dds") || file.EndsWithString(".exr") || file.EndsWithString(".bmp"))
        {
            imageFiles.Append(file);
        }
        else if (file.EndsWithString(".wav") || file.EndsWithString(".mp3") || file.EndsWithString(".ogg"))
        {
            soundFiles.Append(file);
        }
        else if (file.EndsWithString(".namsh"))
        {
            meshFiles.Append(file);
        }
        else if (file.EndsWithString(".nammdl"))
        {
            modelFiles.Append(file);
        }
        else if (file.EndsWithString(".natex"))
        {
            textureFiles.Append(file);
        }
        else if (file.EndsWithString(".namat"))
        {
            materialFiles.Append(file);
        }
        else if (file.EndsWithString(".napar"))
        {
            particleFiles.Append(file);
        }
        else if (file.EndsWithString(".naani"))
        {
            animationFiles.Append(file);
        }
        else if (file.EndsWithString(".naske"))
        {
            skeletonFiles.Append(file);
        }
        else if (file.EndsWithString(".naaud"))
        {
            audioFiles.Append(file);
        }
        else if (file.EndsWithString(".actor"))
        {
            physicsFiles.Append(file);
        }
    }
    if (this->importMode & ImportModes::FBX)
    {
        // import FBX files
        this->logger->Print("\nFBXs -----------\n");
        if (fbxFiles.IsEmpty())
        {
            this->logger->Print("Nothing to import\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < fbxFiles.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(dir + fbxFiles[fileIndex]);
                log.AddEntry(console, "FBX Import", fbxFiles[fileIndex]);
            }
        }
    }

    if (this->importMode & ImportModes::GLTF)
    {
        // import GLTF files
        this->logger->Print("\nGLTFs -----------\n");
        if (gltfFiles.IsEmpty())
        {
            this->logger->Print("Nothing to import\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < gltfFiles.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(dir + gltfFiles[fileIndex]);
                log.AddEntry(console, "GLTF Import", gltfFiles[fileIndex]);
            }
        }
    }

    if (this->importMode & ImportModes::Images)
    {
        // import image files
        this->logger->Print("\nImages -----------\n");
        if (imageFiles.IsEmpty())
        {
            this->logger->Print("Nothing to import\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < imageFiles.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(dir + imageFiles[fileIndex]);
                log.AddEntry(console, "Image Import", imageFiles[fileIndex]);
            }
        }
    }

    if (this->importMode & ImportModes::Sound)
    {
        // import sound files
        this->logger->Print("\nSound -----------\n");
        if (soundFiles.IsEmpty())
        {
            this->logger->Print("Nothing to import\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < soundFiles.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(dir + soundFiles[fileIndex]);
                log.AddEntry(console, "Sound Import", soundFiles[fileIndex]);
            }
        }
    }

    if (this->packageMode & PackageModes::Meshes)
    {
        // export meshes
        this->logger->Print("\nMeshes --------------\n");
        if (meshFiles.IsEmpty())
        {
            this->logger->Print("Nothing to package\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < meshFiles.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(dir + meshFiles[fileIndex]);
                log.AddEntry(console, "Model", meshFiles[fileIndex]);
            }
        }
    }

    if (this->packageMode & PackageModes::Models)
    {
        // export models
        this->logger->Print("\nModels --------------\n");
        if (modelFiles.IsEmpty())
        {
            this->logger->Print("Nothing to package\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < modelFiles.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(dir + modelFiles[fileIndex]);
                log.AddEntry(console, "Model", modelFiles[fileIndex]);
            }
        }
    }

    if (this->packageMode & PackageModes::Textures)
    {
        // export textures
        this->logger->Print("\nTextures ------------\n");
        if (textureFiles.IsEmpty())
        {
            this->logger->Print("Nothing to package\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < textureFiles.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(dir + textureFiles[fileIndex]);
                log.AddEntry(console, "Texture", textureFiles[fileIndex]);
            }
        }
    }

    if (this->packageMode & PackageModes::Materials)
    {
        this->logger->Print("\nMaterials ------------\n");
        if (materialFiles.IsEmpty())
        {
            this->logger->Print("Nothing to package\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < materialFiles.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(dir + materialFiles[fileIndex]);
                log.AddEntry(console, "Material", materialFiles[fileIndex]);
            }
        }
    }

    if (this->packageMode & PackageModes::Particles)
    {
        this->logger->Print("\nParticles ------------\n");
        if (particleFiles.IsEmpty())
        {
            this->logger->Print("Nothing to package\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < particleFiles.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(dir + particleFiles[fileIndex]);
                log.AddEntry(console, "Particle", particleFiles[fileIndex]);
            }
        }
    }

    if (this->packageMode & PackageModes::Animations)
    {
        this->logger->Print("\nAnimations ------------\n");
        if (animationFiles.IsEmpty())
        {
            this->logger->Print("Nothing to package\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < animationFiles.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(dir + animationFiles[fileIndex]);
                log.AddEntry(console, "Animation", animationFiles[fileIndex]);
            }
        }
    }

    if (this->packageMode & PackageModes::Skeletons)
    {
        this->logger->Print("\nSkeletons ------------\n");
        if (skeletonFiles.IsEmpty())
        {
            this->logger->Print("Nothing to package\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < skeletonFiles.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(dir + skeletonFiles[fileIndex]);
                log.AddEntry(console, "Skeleton", skeletonFiles[fileIndex]);
            }
        }
    }

    if (this->packageMode & PackageModes::Audio)
    {
        this->logger->Print("\nAudio ---------------\n");
        if (audioFiles.IsEmpty())
        {
            this->logger->Print("Nothing to package\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < audioFiles.Size(); fileIndex++)
            {
                console->Clear();
                this->ProcessFile(dir + audioFiles[fileIndex]);
                log.AddEntry(console, "Audio", audioFiles[fileIndex]);
            }
        }
    }
    if (this->packageMode & PackageModes::Physics)
    {
        this->logger->Print("\nPhysics -------------\n");
        if (physicsFiles.IsEmpty())
        {
            this->logger->Print("Nothing to package\n");
        }
        else
        {
            for (auto const& file : physicsFiles)
            {
                this->ProcessFile(dir + file);
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
        this->ProcessDir(Util::Format("src:assets/%s/", folders[folderIndex].AsCharPtr()));
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
