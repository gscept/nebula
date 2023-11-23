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
    if (this->textureAttrTable.IsValid()) 
        this->textureAttrTable.Discard();
    this->textureAttrTable.Setup("src:assets/");
    this->textureExporter.SetTextureAttrTable(std::move(this->textureAttrTable));
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
        this->gltfExporter = ToolkitUtil::NglTFExporter::Create();
        this->gltfExporter->SetTextureConverter(&this->textureExporter);
        this->gltfExporter->Open();
        this->gltfExporter->SetForce(this->force || (this->mode & ExportModes::ForceGLTF) != 0);
        this->gltfExporter->SetCategory(category);
        this->gltfExporter->SetLogger(this->logger);
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
                this->gltfExporter->SetFile(files[fileIndex]);
                this->gltfExporter->ExportFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "GLTF", files[fileIndex]);
            }
        }
        this->gltfExporter->Close();
        this->gltfExporter = nullptr;
    }

    if (this->mode & ExportModes::FBX)
    {
        this->logger->Print("\nFBXs ----------------\n");
        // export FBX sources
        Array<String> files = ioServer->ListFiles(assetPath, "*.fbx");
        this->fbxExporter = ToolkitUtil::NFbxExporter::Create();
        this->fbxExporter->Open();
        this->fbxExporter->SetForce(this->force || (this->mode & ExportModes::ForceFBX) != 0);
        this->fbxExporter->SetCategory(category);
        this->fbxExporter->SetLogger(this->logger);
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
                this->fbxExporter->SetFile(files[fileIndex]);
                this->fbxExporter->ExportFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "FBX", files[fileIndex]);
            }
        }
        this->fbxExporter->Close();
        this->fbxExporter = nullptr;
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
                String modelName = files[fileIndex];
                modelName.StripFileExtension();
                modelName = category + "/" + modelName;
                Ptr<ModelConstants> constants = ModelDatabase::Instance()->LookupConstants(modelName, true);
                Ptr<ModelAttributes> attributes = ModelDatabase::Instance()->LookupAttributes(modelName, true);
                Ptr<ModelPhysics> physics = ModelDatabase::Instance()->LookupPhysics(modelName, true);

                this->modelBuilder->SetConstants(constants);
                this->modelBuilder->SetAttributes(attributes);
                this->modelBuilder->SetPhysics(physics);

                // save models and physics

                String modelPath = String::Sprintf("mdl:%s.n3", modelName.AsCharPtr());
                this->logger->Print("%s -> %s\n", Text(URI(assetPath + files[fileIndex]).LocalPath()).Color(TextColor::Blue).AsCharPtr(), Text(URI(modelPath).LocalPath()).Color(TextColor::Green).AsCharPtr());
                this->modelBuilder->SaveN3(modelPath, this->platform);

                String physicsPath = String::Sprintf("phys:%s.actor", modelName.AsCharPtr());
                this->modelBuilder->SaveN3Physics(physicsPath, this->platform);

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
        Array<String> cubes = ioServer->ListDirectories(assetPath, "*.cube");

        this->textureExporter.SetForceFlag(this->force || (this->mode & ExportModes::ForceTextures) != 0);
        this->textureExporter.SetLogger(this->logger);

        this->logger->Print("\nTextures ------------\n");
        Util::String dstDir = Util::String::Sprintf("tex:%s", category.AsCharPtr());
        if (files.IsEmpty() && cubes.IsEmpty())
        {
            this->logger->Print("Nothing to export\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                Util::String dstFile = Util::String::Sprintf("%s/%s", dstDir.AsCharPtr(), files[fileIndex].AsCharPtr());
                dstFile.StripFileExtension();
                this->textureExporter.ConvertTexture(assetPath + files[fileIndex], dstFile, "temp:textureconverter");
                log.AddEntry(console, "Texture", files[fileIndex]);
            }

            for (fileIndex = 0; fileIndex < cubes.Size(); fileIndex++)
            {
                console->Clear();
                Util::String dstFile = Util::String::Sprintf("%s/%s", dstDir.AsCharPtr(), cubes[fileIndex].AsCharPtr());
                dstFile.StripFileExtension();
                this->textureExporter.ConvertCubemap(assetPath + cubes[fileIndex], dstFile, "temp:textureconverter");
                log.AddEntry(console, "Texture", cubes[fileIndex]);
            }
        }
    }

    if (this->mode & ExportModes::Surfaces)
    {
        this->logger->Print("\nSurfaces ------------\n");
        Array<String> files = ioServer->ListFiles(assetPath, "*.sur");
        this->surfaceExporter->SetLogger(this->logger);
        this->surfaceExporter->SetForce(this->force || (this->mode & ExportModes::ForceSurfaces) != 0);
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to export\n");
        }
        else
        {
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                this->surfaceExporter->ExportFile(assetPath + files[fileIndex]);
                log.AddEntry(console, "Surface", files[fileIndex]);
            }
        }
    }

    if (this->mode & ExportModes::Audio)
    {
        this->logger->Print("\nAudio ---------------\n");
        Array<String> files = ioServer->ListFiles(assetPath, "*.wav");
        files.AppendArray(ioServer->ListFiles(assetPath, "*.mp3"));
        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to export\n");
        }
        else
        {
            Util::String dstDir = Util::String::Sprintf("dst:audio/%s", category.AsCharPtr());
            ioServer->CreateDirectory(dstDir);

            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                console->Clear();
                Util::String dstFile = Util::String::Sprintf("%s/%s", dstDir.AsCharPtr(), files[fileIndex].AsCharPtr());
                this->logger->Print("%s -> %s\n", Text(URI(assetPath + files[fileIndex]).LocalPath()).Color(TextColor::Blue).AsCharPtr(), Text(URI(dstFile).LocalPath()).Color(TextColor::Green).AsCharPtr());
                ioServer->CopyFile(assetPath + files[fileIndex], dstFile);
                log.AddEntry(console, "Audio", files[fileIndex]);
            }
        }
    }
    if (this->mode & ExportModes::Physics)
    {
        this->logger->Print("\nPhysics -------------\n");
        Array<String> files = ioServer->ListFiles(assetPath, "*.actor", true);
        Util::String dstDir = Util::String::Sprintf("dst:physics/%s", category.AsCharPtr());

        if (files.IsEmpty())
        {
            this->logger->Print("Nothing to export\n");
        }
        else
        {
            for (auto const& file : files)
            {
                Util::String dstFile = Util::String::Sprintf("%s/%s", dstDir.AsCharPtr(), file.ExtractFileName().AsCharPtr());
                
                if (this->mode & ExportModes::ForcePhysics || NeedsConversion(file, dstFile))
                {
                    Flat::FlatbufferInterface::Compile(file, dstDir, "ACTO");
                    log.AddEntry(console, "Physics", file);
                    this->logger->Print("%s -> %s\n", Text(Format("%s", file.AsCharPtr())).Color(TextColor::Blue).AsCharPtr(), Text(URI(dstFile).LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr());
                }
                else
                {
                    this->logger->Print("Skipping %s\n", Text(file).Color(TextColor::Blue).AsCharPtr());
                }
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
        this->ExportDir(str.ExtractFileName());
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
