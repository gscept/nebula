//------------------------------------------------------------------------------
//  assetexporter.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "assetexporter.h"
#include "io/ioserver.h"
#include "io/assignregistry.h"
#include "toolkitconsolehandler.h"

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
    this->textureExporter.SetTexAttrTablePath("src:assets/");
    this->textureExporter.Setup(this->logger);
}

//------------------------------------------------------------------------------
/**
*/
void
AssetExporter::Close()
{
	this->surfaceExporter->Close();
	this->surfaceExporter = 0;
    this->modelBuilder = 0;
    this->textureExporter.Discard();
    ExporterBase::Close();
}


//------------------------------------------------------------------------------
/**
*/
void
AssetExporter::ExportSystem()
{
	String origSrc = AssignRegistry::Instance()->GetAssign("src");
	AssignRegistry::Instance()->SetAssign(Assign("src", "toolkit:work"));
	this->ExportDir("system");
	this->ExportDir("lighting");
	this->ExportDir("placeholder");
	AssignRegistry::Instance()->SetAssign(Assign("src", origSrc));	
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
    n_printf("Exporting asset directory '%s'\n", category.AsCharPtr());

    
    IndexT fileIndex;
	ToolLog log(category);
	Ptr<ToolkitUtil::ToolkitConsoleHandler> console = ToolkitUtil::ToolkitConsoleHandler::Instance();
    if (this->mode & ExportModes::FBX)
    {
		console->Clear();
        // export FBX sources
        Array<String> files = IoServer::Instance()->ListFiles(assetPath, "*.fbx");
		this->fbxExporter = ToolkitUtil::NFbxExporter::Create();
		this->fbxExporter->Open();
		this->fbxExporter->SetForce(this->force || (this->mode & ExportModes::ForceFBX) != 0);
		this->fbxExporter->SetCategory(category);
		log.AddEntry(console, "FBX", category);
        for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
		{
			console->Clear();
            this->fbxExporter->SetFile(files[fileIndex]);
            this->fbxExporter->ExportFile(assetPath + files[fileIndex]);			
			log.AddEntry(console, "FBX", files[fileIndex]);
        }
		this->fbxExporter->Close();
		this->fbxExporter = 0;
    }    

    if (this->mode & ExportModes::Models)
    {
        // export models
        Array<String> files = IoServer::Instance()->ListFiles(assetPath, "*.attributes");
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
            this->modelBuilder->SaveN3(modelPath, this->platform);
            String physicsPath = String::Sprintf("phys:%s.np3", modelName.AsCharPtr());
            this->modelBuilder->SaveN3Physics(physicsPath, this->platform);
			log.AddEntry(console, "Model", files[fileIndex]);			
        }
    }

    if (this->mode & ExportModes::Textures)
    {
        // export textures
        Array<String> files = IoServer::Instance()->ListFiles(assetPath, "*.tga");
        files.AppendArray(IoServer::Instance()->ListFiles(assetPath, "*.bmp"));
        files.AppendArray(IoServer::Instance()->ListFiles(assetPath, "*.dds"));
        files.AppendArray(IoServer::Instance()->ListFiles(assetPath, "*.psd"));
        files.AppendArray(IoServer::Instance()->ListFiles(assetPath, "*.png"));
        files.AppendArray(IoServer::Instance()->ListFiles(assetPath, "*.jpg"));
		this->textureExporter.SetForceFlag(this->force || (this->mode & ExportModes::ForceTextures) != 0);
        for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
        {
			console->Clear();
            this->textureExporter.SetDstDir("tex:");
            this->textureExporter.ConvertTexture(assetPath + files[fileIndex], "temp:textureconverter");
			log.AddEntry(console, "Texture", files[fileIndex]);			
        }
    }

	if (this->mode & ExportModes::Surfaces)
	{
		Array<String> files = IoServer::Instance()->ListFiles(assetPath, "*.sur");
		this->surfaceExporter->SetForce(this->force || (this->mode & ExportModes::ForceSurfaces) != 0);
		for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
		{
			console->Clear();
			this->surfaceExporter->ExportFile(assetPath + files[fileIndex]);
			log.AddEntry(console, "Surface", files[fileIndex]);			
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
		const Util::String & str = *iter;
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