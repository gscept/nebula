//------------------------------------------------------------------------------
//  assetexporter.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "assetexporter.h"
#include "io/ioserver.h"
#include "io/assignregistry.h"
#include "toolkit-common/toolkitconsolehandler.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "toolkit-common/text.h"
#include "io/jsonreader.h"

using namespace Util;
using namespace IO;
namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::AssetExporter, 'ASEX', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
AssetExporter::AssetExporter() :
    mode(All)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AssetExporter::~AssetExporter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
AssetExporter::Open()
{
    ExporterBase::Open();
    this->surfaceExporter = ToolkitUtil::SurfaceExporter::Create();
    this->surfaceExporter->Open();
    this->modelBuilder = ToolkitUtil::ModelBuilder::Create();
    this->textureExporter.Setup();
}

//------------------------------------------------------------------------------
/**
*/
void
AssetExporter::Close()
{
    this->surfaceExporter->Close();
    this->surfaceExporter = nullptr;
    this->modelBuilder = nullptr;
    this->textureExporter.Discard();
    this->textureAttrTable.Discard();
    ExporterBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
AssetExporter::UpdateSource()
{
    this->RecurseValidateIntermediates("intermediate:");

    if (this->textureAttrTable.IsValid()) 
        this->textureAttrTable.Discard();
    this->textureAttrTable.Setup("src:assets/");
    this->textureExporter.SetTextureAttrTable(std::move(this->textureAttrTable));
}

//------------------------------------------------------------------------------
/**
*/
void
AssetExporter::ExportFile(const IO::URI& file)
{
    IoServer* ioServer = IoServer::Instance();
    Util::String const ext = file.AsString().GetFileExtension();
    Util::String const fileName = file.AsString().ExtractFileName();

    if ((this->mode & ExportModes::GLTF) && (ext == "gltf" || ext == "glb"))
    {
        this->gltfExporter = ToolkitUtil::NglTFExporter::Create();
        this->gltfExporter->SetTextureConverter(&this->textureExporter);
        this->gltfExporter->Open();
        this->gltfExporter->SetForce(this->force || (this->mode & ExportModes::ForceGLTF) != 0);
        this->gltfExporter->SetCategory(category);
        this->gltfExporter->SetLogger(this->logger);
        this->gltfExporter->SetFile(fileName);
        this->gltfExporter->ExportFile(file);
        this->gltfExporter->Close();
        this->gltfExporter = nullptr;
    }
    else if ((this->mode & ExportModes::FBX) && ext == "fbx")
    {
        this->fbxExporter = ToolkitUtil::NFbxExporter::Create();
        this->fbxExporter->Open();
        this->fbxExporter->SetForce(this->force || (this->mode & ExportModes::ForceFBX) != 0);
        this->fbxExporter->SetCategory(category);
        this->fbxExporter->SetLogger(this->logger);
        this->fbxExporter->SetFile(fileName);
        this->fbxExporter->ExportFile(file);
        this->fbxExporter->Close();
        this->fbxExporter = nullptr;
    }
    else if ((this->mode & ExportModes::Models) && ext == "attributes")
    {
        String modelName = fileName;
        modelName.StripFileExtension();
        modelName = category + "/" + modelName;
        Ptr<ModelConstants> constants = ModelDatabase::Instance()->LookupConstants(modelName, true);
        Ptr<ModelAttributes> attributes = ModelDatabase::Instance()->LookupAttributes(modelName, true);
        Ptr<ModelPhysics> physics = ModelDatabase::Instance()->LookupPhysics(modelName, true);

        this->modelBuilder->SetConstants(constants);
        this->modelBuilder->SetAttributes(attributes);
        this->modelBuilder->SetPhysics(physics);

        String modelPath = String::Sprintf("mdl:%s.n3", modelName.AsCharPtr());
        this->logger->Print(
            "%s -> %s\n",
            Text(file.LocalPath()).Color(TextColor::Blue).AsCharPtr(),
            Text(URI(modelPath).LocalPath()).Color(TextColor::Green).AsCharPtr()
        );
        this->modelBuilder->SaveN3(modelPath, this->platform);

        String physicsPath = String::Sprintf("phys:%s.actor", modelName.AsCharPtr());
        this->modelBuilder->SaveN3Physics(physicsPath, this->platform);
    }
    else if ((this->mode & ExportModes::Textures) &&
             (
                 ext == "jpg" ||
                 ext == "tga" ||
                 ext == "bmp" ||
                 ext == "dds" ||
                 ext == "png" ||
                 ext == "jpg" ||
                 ext == "exr" ||
                 ext == "tif" ||
                 ext == "cube"
              ))
    {
        this->textureExporter.SetForceFlag(this->force || (this->mode & ExportModes::ForceTextures) != 0);
        this->textureExporter.SetLogger(this->logger);

        Util::String dstDir = Util::String::Sprintf("tex:%s", category.AsCharPtr());
        
        Util::String dstFile = Util::String::Sprintf("%s/%s", dstDir.AsCharPtr(), fileName.AsCharPtr());
        dstFile.StripFileExtension();
        if (ext == "cube")
            this->textureExporter.ConvertCubemap(file.AsString(), dstFile, "temp:textureconverter");
        else
            this->textureExporter.ConvertTexture(file.AsString(), dstFile, "temp:textureconverter");
    }
    else if ((this->mode & ExportModes::Surfaces) && ext == "sur")
    {
        this->surfaceExporter->SetLogger(this->logger);
        this->surfaceExporter->SetForce(this->force || (this->mode & ExportModes::ForceSurfaces) != 0);
        this->surfaceExporter->ExportFile(file);
    }
    else if ((this->mode & ExportModes::Audio) &&
             (
                ext == "mp3" ||
                ext == "wav" ||
                ext == "ogg"
             ))
    {
        Util::String dstDir = Util::String::Sprintf("dst:audio/%s", category.AsCharPtr());
        ioServer->CreateDirectory(dstDir);
        Util::String dstFile = Util::String::Sprintf("%s/%s", dstDir.AsCharPtr(), fileName.AsCharPtr());
        this->logger->Print(
            "%s -> %s\n",
            Text(file.LocalPath()).Color(TextColor::Blue).AsCharPtr(),
            Text(URI(dstFile).LocalPath()).Color(TextColor::Green).AsCharPtr()
        );
        ioServer->CopyFile(file, dstFile);
    }
    else if ((this->mode & ExportModes::Physics) && ext == "actor")
    {
        Util::String dstDir = Util::String::Sprintf("dst:physics/%s", category.AsCharPtr());
        Util::String dstFile = Util::String::Sprintf("%s/%s", dstDir.AsCharPtr(), fileName.AsCharPtr());
        if (this->mode & ExportModes::ForcePhysics || NeedsConversion(file.AsString(), dstFile))
        {
            this->logger->Print(
                "%s -> %s\n",
                Text(Format("%s", file.AsString().AsCharPtr())).Color(TextColor::Blue).AsCharPtr(),
                Text(URI(dstFile).LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr()
            );
            Flat::FlatbufferInterface::Compile(file, dstDir, "ACTO");
        }
        else
        {
            this->logger->Print("Skipping %s\n", Text(file.AsString()).Color(TextColor::Blue).AsCharPtr());
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetExporter::ExportDir(const Util::String& category)
{
    String assetPath = String::Sprintf("src:assets/%s/", category.AsCharPtr());
    this->ExportFolder(assetPath, category);
}

//------------------------------------------------------------------------------
/**
*/
void
AssetExporter::ExportFolder(const Util::String& assetPath, const Util::String& category)
{
    n_printf("\n----------------- Exporting asset directory %s -----------------\n", Text(URI(assetPath).LocalPath()).Color(TextColor::Blue).Style(FontMode::Bold).AsCharPtr());
    IoServer* ioServer = IoServer::Instance();

    IndexT fileIndex;
    ToolLog log(category);
    Ptr<ToolkitUtil::ToolkitConsoleHandler> console = ToolkitUtil::ToolkitConsoleHandler::Instance();

    if (this->mode & ExportModes::GLTF)
    {
        console->Clear();
        // export GLTF sources

        this->logger->Print("\nGLTFs ------------\n");
        Array<String> files = ioServer->ListFiles(assetPath, "*.gltf");
        files.AppendArray(ioServer->ListFiles(assetPath, "*.glb"));
        log.AddEntry(console, "GLTF", category);
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to export\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                this->ExportFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "GLTF", files[fileIndex]);
            }
        }
    }

    if (this->mode & ExportModes::FBX)
    {
        this->logger->Print("\nFBXs ----------------\n");

        // export FBX sources
        Array<String> files = ioServer->ListFiles(assetPath, "*.fbx");
        log.AddEntry(console, "FBX", category);
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to export\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                this->ExportFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "FBX", files[fileIndex]);
            }
        }
    }

    if (this->mode & ExportModes::Models)
    {
        // export models
        this->logger->Print("\nModels --------------\n");
        Array<String> files = ioServer->ListFiles(assetPath, "*.attributes");
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to export\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                this->ExportFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "Model", files[fileIndex]);
            }
        }
    }

    if (this->mode & ExportModes::Textures)
    {
        // export textures
        Array<String> files = ioServer->ListFiles(assetPath, "*.tga");
        files.AppendArray(ioServer->ListFiles(assetPath, "*.bmp"));
        files.AppendArray(ioServer->ListFiles(assetPath, "*.dds"));
        files.AppendArray(ioServer->ListFiles(assetPath, "*.png"));
        files.AppendArray(ioServer->ListFiles(assetPath, "*.jpg"));
        files.AppendArray(ioServer->ListFiles(assetPath, "*.exr"));
        files.AppendArray(ioServer->ListFiles(assetPath, "*.tif"));
        files.AppendArray(ioServer->ListDirectories(assetPath, "*.cube"));

        this->logger->Print("\nTextures ------------\n");
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to export\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                this->ExportFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "Texture", files[fileIndex]);
            }
        }
    }

    if (this->mode & ExportModes::Surfaces)
    {
        this->logger->Print("\nSurfaces ------------\n");
        Array<String> files = ioServer->ListFiles(assetPath, "*.sur");
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to export\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                this->ExportFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "Surface", files[fileIndex]);
            }
        }
    }

    if (this->mode & ExportModes::Audio)
    {
        this->logger->Print("\nAudio ---------------\n");
        Array<String> files = ioServer->ListFiles(assetPath, "*.wav");
        files.AppendArray(ioServer->ListFiles(assetPath, "*.mp3"));
        files.AppendArray(ioServer->ListFiles(assetPath, "*.ogg"));
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to export\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                this->ExportFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "Audio", files[fileIndex]);
            }
        }
    }
    if (this->mode & ExportModes::Physics)
    {
        this->logger->Print("\nPhysics -------------\n");
        Array<String> files = ioServer->ListFiles(assetPath, "*.actor", true);
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to export\n");
        }
        else
        {
            for (auto const& file : files)
            {
                this->ExportFile(assetPath + file);
                log.AddEntry(console, "Physics", file);
            }
        }
    }
    this->messages.Append(log);
}

//------------------------------------------------------------------------------
/**
*/
void
AssetExporter::ExportAll()
{
    IndexT folderIndex;
    Array<String> folders = IoServer::Instance()->ListDirectories("src:assets/", "*");
    for (folderIndex = 0; folderIndex < folders.Size(); folderIndex++)
    {
        this->ExportDir(folders[folderIndex]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetExporter::ExportList(const Util::Array<Util::String>& files)
{
    for (Array<String>::Iterator iter = files.Begin(); iter != files.End(); iter++)
    {
        const Util::String& str = *iter;
        if (IO::IoServer::Instance()->FileExists(str))
        {
            this->ExportFile(str);
        }
        else if (IO::IoServer::Instance()->DirectoryExists(str))
        {
            Array<String> filesInFolder = IoServer::Instance()->ListFiles(str, "*");
            filesInFolder.AppendArray(IoServer::Instance()->ListDirectories(str, "*"));
            for (auto& f : filesInFolder)
                f = str + "/" + f;
            this->ExportList(filesInFolder);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetExporter::SetExportMode(unsigned int mode)
{
    this->mode = mode;
}

} // namespace ToolkitUtil
