//------------------------------------------------------------------------------
//  physicsexporter.cc
//  (C) 2011 gscept
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physicsexporter.h"
#include "foundation\io\ioserver.h"
#include "toolkitutil\n3util\n3xmlextractor.h"
#include "io\xmlwriter.h"

using namespace IO;
using namespace Util;

namespace ToolkitUtil
{
	__ImplementClass(ToolkitUtil::PhysicsExporter, 'PHEX', Base::ExporterBase);

//------------------------------------------------------------------------------
/**
*/
PhysicsExporter::PhysicsExporter()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
PhysicsExporter::~PhysicsExporter()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
PhysicsExporter::Open()
{
	ExporterBase::Open();
}

//------------------------------------------------------------------------------
/**
*/
void 
PhysicsExporter::Close()
{
	ExporterBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void 
PhysicsExporter::ExportAll()
{
	String workDir = "proj:work/models";
	Array<String> directories = IoServer::Instance()->ListDirectories(workDir, "*");
	for (int directoryIndex = 0; directoryIndex < directories.Size(); directoryIndex++)
	{
		String category = workDir + "/" + directories[directoryIndex];
		Array<String> files = IoServer::Instance()->ListFiles(category, "*.xml");
		for (int fileIndex = 0; fileIndex < files.Size(); fileIndex++)
		{
			this->ExportFile(category + "/" + files[fileIndex]);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
PhysicsExporter::ExportDir( const String& category )
{
	String categoryDir = "proj:work/models" + category;
	Array<String> files = IoServer::Instance()->ListFiles(categoryDir, "*.xml");
	for (int fileIndex = 0; fileIndex < files.Size(); fileIndex++)
	{
		this->ExportFile(category + "/" + files[fileIndex]);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
PhysicsExporter::ExportFile( const IO::URI& file )
{
	Ptr<IoServer> ioServer = IoServer::Instance();
	String fileName = file.GetHostAndLocalPath().ExtractFileName();
	fileName.StripFileExtension();
	String category = file.GetHostAndLocalPath().ExtractLastDirName();

	n_printf("Exporting physics: %s/%s\n", category.AsCharPtr(), file.GetHostAndLocalPath().AsCharPtr());
	this->Progress(1, "Exporting: " + file.AsString());

	/// create export physics folder if it doesn't exist already
	n_assert(ioServer->CreateDirectory("export:physics/" + category));

	bool needsDummyPhysics = this->NeedsDummyPhysics(category, fileName);
	if (needsDummyPhysics)
	{
		n_printf("No physics resource found, creating from bounding box...\n");
		Ptr<Stream> modelStream = ioServer->CreateStream("proj:work/models/" + category + "/" + fileName + ".xml");
		modelStream->Open();
		Ptr<N3XmlExtractor> extractor = N3XmlExtractor::Create();
		extractor->SetStream(modelStream);
		extractor->Open();
		Math::bbox boundingBox;
		extractor->ExtractSceneBoundingBox(boundingBox);
		extractor->Close();
		modelStream->Close();

		this->CreateDummyPhysics(boundingBox, category, fileName);
	}
	else
	{
		/// we know the .bullet in work is newer than the original
		bool needsConversion = this->NeedsConversion("proj:work/physics/" + category + "/" + fileName + ".xml");

		/// so we only need to copy from work to export
		ioServer->CopyFile("proj:work/physics/" + category + "/" + fileName + ".xml", "phys:" + category + "/" + fileName + ".xml");
	}
	
	n_printf("Done!\n\n");
	
}

//------------------------------------------------------------------------------
/**
*/
bool 
PhysicsExporter::NeedsConversion( const String& path )
{
	String category = path.ExtractLastDirName();
	String file = path.ExtractFileName();
	file.StripFileExtension();

	String dst = "phys:" + category + "/" + file + ".xml";
	return this->NeedsConversion(path, dst);
}

//------------------------------------------------------------------------------
/**
*/
bool 
PhysicsExporter::NeedsConversion( const String& src, const String& dst )
{
	if (this->force)
	{
		return true;
	}

	Ptr<IoServer> ioServer = IoServer::Instance();
	if (ioServer->FileExists(dst) && ioServer->FileExists(src))
	{
		FileTime srcFileTime = ioServer->GetFileWriteTime(src);
		FileTime dstFileTime = ioServer->GetFileWriteTime(dst);
		if (dstFileTime > srcFileTime)
		{
			return false;
		}
	}

	// fallthrough
	return true;
}

//------------------------------------------------------------------------------
/**
*/
bool 
PhysicsExporter::NeedsDummyPhysics( const String& category, const String& file )
{
	IO::URI uri("proj:work/physics/" + category + "/" + file + ".bullet");
	return !IoServer::Instance()->FileExists(uri);
}

//------------------------------------------------------------------------------
/**
*/
void 
PhysicsExporter::CreateDummyPhysics( const Math::bbox& box, const String& category, const String& file )
{
	Ptr<IO::XmlWriter> writer = IO::XmlWriter::Create();

	Ptr<Stream> physicsStream = IoServer::Instance()->CreateStream("phys:" + category + "/" + file + ".xml");
	
	physicsStream->SetAccessMode(Stream::WriteAccess);
	physicsStream->Open();

	writer->SetStream(physicsStream);
	writer->Open();

	writer->BeginNode("physics");
	writer->BeginNode("collidergroup");
	writer->SetString("name","dummybox");
	writer->BeginNode("collider");
	writer->SetInt("type",1);
	writer->SetFloat4("halfWidth",box.extents());
	Math::matrix44 trans;
	trans.translate(Math::vector(box.center()));
	writer->SetMatrix44("transform",trans);
	writer->EndNode();
	writer->EndNode();
	writer->BeginNode("body");
	writer->SetFloat("mass",0.0f);
	writer->SetString("name","dummybody");
	writer->SetInt("type",0);
	writer->SetInt("flags",0);
	writer->SetString("collider","dummybox");
	writer->EndNode();
	writer->EndNode();
	writer->Close();
	physicsStream->Close();
}

} // namespace Toolkit