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

#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/model.h"
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
        ToolkitUtil::TextureResourceT texture = ToolkitUtil::SetupTextureImportSettingsFromPath(file);
        ToolkitUtil::ImportTexture(file, this->folder, texture);
    }

    if ((this->importMode & ImportModes::Sound) && (ext == "wav" || ext == "ogg" || ext == "mp3"))
    {
        ToolkitUtil::ImportAudio(file, this->folder);
    }

    Util::String packageFolder = this->folder.StripSubstring("src:assets/");
    if (this->packageMode & PackageModes::Assets && ext == "nasset")
    {
        Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
        stream->SetAccessMode(IO::Stream::ReadAccess);
        if (stream->Open())
        {
            const void* data = stream->MemoryMap();

            ToolkitUtil::ModelAssetT modelAsset;
            Flat::FlatbufferInterface::DeserializeFlatbuffer<ToolkitUtil::ModelAsset>(modelAsset, (const uint8_t*)data);

            Util::String fileNameNoExt = file.LocalPath().ExtractFileName();
            fileNameNoExt.StripFileExtension();
            if (modelAsset.scene != nullptr)
            {
                Util::String dstFolder = Util::String::Sprintf("mdl:%s", packageFolder.AsCharPtr());
                IO::CreateDirectory(dstFolder);
                Util::String dstFile = Util::String::Sprintf("%s%s.n3", dstFolder.AsCharPtr(), fileNameNoExt.AsCharPtr());
                Util::String srcFile = Util::String::Sprintf("%s%s.nasset", dstFolder.AsCharPtr(), fileNameNoExt.AsCharPtr());
                ToolkitUtil::PackageModel(modelAsset.scene.get(), fileNameNoExt, dstFolder, ToolkitUtil::Platform::Code::Win32, this->logger);

                Util::String srcFileNoExt = srcFile;
                srcFileNoExt.StripFileExtension();
                Util::String urn = Util::String::Sprintf("urn:%s", srcFileNoExt.AsCharPtr());
                this->UpdateResourceMapping(urn, file.AsString(), dstFile);
            }
            if (modelAsset.mesh != nullptr)
            {
                Util::String dstFolder = Util::String::Sprintf("msh:%s", packageFolder.AsCharPtr());
                IO::CreateDirectory(dstFolder);
                Util::String dstFile = Util::String::Sprintf("%s%s.nvx", dstFolder.AsCharPtr(), fileNameNoExt.AsCharPtr());
                Util::String srcFile = Util::String::Sprintf("%s%s.nasset", dstFolder.AsCharPtr(), fileNameNoExt.AsCharPtr());
                ToolkitUtil::PackageMesh(modelAsset.mesh.get(), fileNameNoExt, dstFolder, ToolkitUtil::Platform::Code::Win32, this->logger);
                
                Util::String srcFileNoExt = srcFile;
                srcFileNoExt.StripFileExtension();
                Util::String urn = Util::String::Sprintf("urn:%s", srcFileNoExt.AsCharPtr());
                this->UpdateResourceMapping(urn, file.AsString(), dstFile);
            }
            if (modelAsset.animation != nullptr)
            {
                Util::String dstFolder = Util::String::Sprintf("ani:%s", packageFolder.AsCharPtr());
                IO::CreateDirectory(dstFolder);
                Util::String dstFile = Util::String::Sprintf("%s%s.nax", dstFolder.AsCharPtr(), fileNameNoExt.AsCharPtr());
                Util::String srcFile = Util::String::Sprintf("%s%s.nasset", dstFolder.AsCharPtr(), fileNameNoExt.AsCharPtr());
                ToolkitUtil::PackageAnimation(modelAsset.animation.get(), fileNameNoExt, dstFolder, ToolkitUtil::Platform::Code::Win32, this->logger);
                                
                Util::String srcFileNoExt = srcFile;
                srcFileNoExt.StripFileExtension();
                Util::String urn = Util::String::Sprintf("urn:%s", srcFileNoExt.AsCharPtr());
                this->UpdateResourceMapping(urn, file.AsString(), dstFile);
            }
            if (modelAsset.skeleton != nullptr)
            {
                Util::String dstFolder = Util::String::Sprintf("ske:%s", packageFolder.AsCharPtr());
                IO::CreateDirectory(dstFolder);
                Util::String dstFile = Util::String::Sprintf("%s%s.nsk", dstFolder.AsCharPtr(), fileNameNoExt.AsCharPtr());
                Util::String srcFile = Util::String::Sprintf("%s%s.nasset", dstFolder.AsCharPtr(), fileNameNoExt.AsCharPtr());
                ToolkitUtil::PackageSkeleton(modelAsset.skeleton.get(), fileNameNoExt, dstFolder, ToolkitUtil::Platform::Code::Win32, this->logger);
                                
                Util::String srcFileNoExt = srcFile;
                srcFileNoExt.StripFileExtension();
                Util::String urn = Util::String::Sprintf("urn:%s", srcFileNoExt.AsCharPtr());
                this->UpdateResourceMapping(urn, file.AsString(), dstFile);
            }
            if (modelAsset.physics != nullptr)
            {
                Util::String dstFolder = Util::String::Sprintf("phys:%s", packageFolder.AsCharPtr());
                IO::CreateDirectory(dstFolder);
                Util::String dstFile = Util::String::Sprintf("%s%s.actor", dstFolder.AsCharPtr(), fileNameNoExt.AsCharPtr());
                Util::String srcFile = Util::String::Sprintf("%s%s.nasset", dstFolder.AsCharPtr(), fileNameNoExt.AsCharPtr());
                ToolkitUtil::PackagePhysics(modelAsset.physics.get(), fileNameNoExt, dstFolder, ToolkitUtil::Platform::Code::Win32, this->logger);
                                
                Util::String srcFileNoExt = srcFile;
                srcFileNoExt.StripFileExtension();
                Util::String urn = Util::String::Sprintf("urn:%s", srcFileNoExt.AsCharPtr());
                this->UpdateResourceMapping(urn, file.AsString(), dstFile);
            }
        }
    }
    if ((this->packageMode & PackageModes::Textures) && ext == "natex")
    {
        Util::String dstFolder = Util::String::Sprintf("tex:%s", packageFolder.AsCharPtr());
        Util::String dstFile = Util::String::Sprintf("%s%s", dstFolder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackageTextureFile(file, dstFolder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
    else if ((this->packageMode & PackageModes::Materials) && ext == "namat")
    {
        Util::String dstFolder = Util::String::Sprintf("mat:%s", packageFolder.AsCharPtr());
        Util::String dstFile = Util::String::Sprintf("%s%s", dstFolder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackageMaterialFile(file, dstFolder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
    else if ((this->packageMode & PackageModes::Particles) && ext == "napar")
    {
        Util::String dstFolder = Util::String::Sprintf("par:%s", packageFolder.AsCharPtr());
        Util::String dstFile = Util::String::Sprintf("%s%s", dstFolder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackageParticleFile(file, dstFolder, ToolkitUtil::Platform::Code::Win32, this->logger);
        Util::String urn = Util::String::Sprintf("urn:%s", dstFile.AsCharPtr());
        this->UpdateResourceMapping(urn, file.AsString(), dstFile);
    }
    else if ((this->packageMode & PackageModes::Audio) && ext == "naaud")
    {
        Util::String dstFolder = Util::String::Sprintf("audio:%s", packageFolder.AsCharPtr());
        Util::String dstFile = Util::String::Sprintf("%s%s", dstFolder.AsCharPtr(), fileName.AsCharPtr());
        ToolkitUtil::PackageAudioFile(file, dstFolder, ToolkitUtil::Platform::Code::Win32, this->logger);
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

    Array<String> fbxFiles, gltfFiles, imageFiles, soundFiles, modelFiles, textureFiles, materialFiles, particleFiles, audioFiles;
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
        else if (file.EndsWithString(".nasset"))
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
        else if (file.EndsWithString(".naaud"))
        {
            audioFiles.Append(file);
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
                this->ProcessFile(dir + fbxFiles[fileIndex]);
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
                this->ProcessFile(dir + gltfFiles[fileIndex]);
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
                this->ProcessFile(dir + imageFiles[fileIndex]);
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
                this->ProcessFile(dir + soundFiles[fileIndex]);
            }
        }
    }

    if (this->packageMode & PackageModes::Assets)
    {
        // export models
        this->logger->Print("\nAssets --------------\n");
        if (modelFiles.IsEmpty())
        {
            this->logger->Print("Nothing to package\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < modelFiles.Size(); fileIndex++)
            {
                this->ProcessFile(dir + modelFiles[fileIndex]);
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
                this->ProcessFile(dir + textureFiles[fileIndex]);
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
                this->ProcessFile(dir + materialFiles[fileIndex]);
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
                this->ProcessFile(dir + particleFiles[fileIndex]);
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
                this->ProcessFile(dir + audioFiles[fileIndex]);
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
