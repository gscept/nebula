//------------------------------------------------------------------------------
//  guiexporter.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "gui/guiexporter.h"
#include "attr/attributedefinitionbase.h"
#include "db/dbfactory.h"
#include "attr/attrid.h"
#include "db/sqlite3/sqlite3factory.h"
#include "io/ioserver.h"
#include "db/sqlite3/sqlite3command.h"
#include "io/xmlreader.h"
#include "io/stream.h"
#include "db/valuetable.h"
#include "db/dataset.h"
#include "db/table.h"
#include "io/filestream.h"

using namespace Db;
using namespace IO;
using namespace Attr;
using namespace Util;

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::CEGuiExporter, 'CGEX', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
CEGuiExporter::CEGuiExporter()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
CEGuiExporter::~CEGuiExporter()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
CEGuiExporter::Open()
{
	ExporterBase::Open();

	IO::AssignRegistry::Instance()->SetAssign(IO::Assign("ceui", "export:/ceui/datafiles/"));
	//IO::AssignRegistry::Instance()->SetAssign(IO::Assign("ceuiwork", "work:ceui/"));
}

//------------------------------------------------------------------------------
/**
*/
void 
CEGuiExporter::Close()
{
	ExporterBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void 
CEGuiExporter::ExportAll(ToolkitUtil::TextureConverter textureConverter, ToolkitUtil::Logger logger)
{
	Util::String dstFontsDir =		"ceui:fonts/";
	Util::String dstImagesetsDir =	"ceui:imagesets/";
	Util::String dstLayoutsDir =	"ceui:layouts/";
	Util::String dstLooknfeelsDir =	"ceui:looknfeel/";
	Util::String dstSchemesDir =	"ceui:schemes/";
	Util::String dstLuaScrpitsDir = "ceui:lua_scripts/";
	Util::String dstXmlSchemasDir =	"ceui:xml_schemas/";
	Util::String dstAnimationsDir = "ceui:animations/";

	Util::String srcFontsDir;
	Util::String srcImagesetsDir;
	Util::String srcLayoutsDir;
	Util::String srcLooknfeelsDir;
	Util::String srcSchemesDir;
	Util::String srcXmlSchemasDir;
	Util::String srcLuaScriptsDir;
	Util::String srcAnimationsDir;

	if(!IO::IoServer::Instance()->DirectoryExists("export:/ceui/"))	//create export ceui folder if it doesn't exist
		IO::IoServer::Instance()->CreateDirectory("export:/ceui/");

	Util::Array<Util::String> projFiles = IO::IoServer::Instance()->ListFiles("proj:work/ceui/", "*.project");

	if(projFiles.IsEmpty())
	{
		n_printf("ERROR: no .project file was found in \"proj:work/ceui/\"\n");
		this->SetHasErrors(true);
		return;
	}

	n_printf("Loading and extracting data from '.project' file..\n");

	//assume there is only one .project file. Load project file
	IO::URI file = IO::URI("proj:work/ceui/" + projFiles[0]);
	Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
	Ptr<IO::XmlReader> xmlReader = IO::XmlReader::Create();

	Util::String s;
	s.Format("Could not open %s", file.GetHostAndLocalPath().AsCharPtr());
	n_assert2(stream->Open(), s.AsCharPtr());
	xmlReader->SetStream(stream);
	n_assert(xmlReader->Open());

	// extract src directories from .project file
	xmlReader->SetToFirstChild("Project");

	Util::String baseDir = xmlReader->GetString("baseDirectory");

	srcFontsDir = "proj:work/ceui/" + baseDir + "/" + xmlReader->GetString("fontsPath") + "/";
	srcImagesetsDir = "proj:work/ceui/" + baseDir + "/" + xmlReader->GetString("imagesetsPath") + "/";
	srcLayoutsDir =	"proj:work/ceui/" + baseDir + "/" + xmlReader->GetString("layoutsPath") + "/";
	srcLooknfeelsDir = "proj:work/ceui/" + baseDir + "/" + xmlReader->GetString("looknfeelsPath") + "/";
	srcSchemesDir =	"proj:work/ceui/" + baseDir + "/" + xmlReader->GetString("schemesPath") + "/";
	srcXmlSchemasDir = "proj:work/ceui/" + baseDir + "/" + xmlReader->GetString("xmlSchemasPath") + "/";
	srcLuaScriptsDir = "proj:work/ceui/" + baseDir + "/" + "lua_scripts/"; //assume
	srcAnimationsDir = "proj:work/ceui/" + baseDir + "/" + "animations/"; //assume


	//copy files
	n_printf("Exporting Font files...\n");
	this->CopyFiles(srcFontsDir, dstFontsDir);
	n_printf("Exporting Imageset files...\n");
	this->CopyImagesetFiles(srcImagesetsDir, dstImagesetsDir); //special case for .imageset files (edit imagefileextension to .dds)
	n_printf("Exporting Layout files...\n");
	this->CopyFiles(srcLayoutsDir, dstLayoutsDir);
	n_printf("Exporting Looknfeel files...\n");
	this->CopyFiles(srcLooknfeelsDir, dstLooknfeelsDir);
	n_printf("Exporting Schemes files...\n");
	this->CopyFiles(srcSchemesDir, dstSchemesDir);
	n_printf("Exporting Xmlschemas files...\n");
	this->CopyFiles(srcXmlSchemasDir, dstXmlSchemasDir);
	n_printf("Exporting Luascript files...\n");
	this->CopyFiles(dstLuaScrpitsDir, srcLuaScriptsDir);
	n_printf("Exporting Animation files...\n");
	this->CopyFiles(srcAnimationsDir, dstAnimationsDir);


	// export all textures in imageset folder
	Util::Array<Util::String> textureFiles = IO::IoServer::Instance()->ListFiles(srcImagesetsDir, "*", true);

	textureConverter.SetSrcDir(srcImagesetsDir);
	textureConverter.SetDstDir("ceui:"); //folder structure fixes where exported images end up.
	if (!textureConverter.Setup(logger))
	{
		n_printf("ERROR: failed to setup texture converter!\n");
		this->SetHasErrors(true);
	}
	n_printf("Converting texture files...\n");
	if(!textureConverter.ConvertFiles(textureFiles))
	{
		n_printf("ERROR: Textures not exported.\n");
		this->SetHasErrors(true);
	}

	textureConverter.Discard();

	xmlReader->Close();
	stream->Close();

	n_printf("\nGUI exporting Done!\n\n");

}

//------------------------------------------------------------------------------------
/**
*/
void
CEGuiExporter::CopyFiles( Util::String srcDir, Util::String dstDir, Util::String filter )
{
	Util::Array<Util::String> fileList = IO::IoServer::Instance()->ListFiles(srcDir, filter);

	if(!IO::IoServer::Instance()->DirectoryExists(dstDir))
		IO::IoServer::Instance()->CreateDirectory(dstDir);

	IndexT index;
	for (index = 0; index < fileList.Size(); index++)
	{
		Util::String fileName = fileList[index].ExtractFileName();
		n_assert(IO::IoServer::Instance()->CopyFile(srcDir + fileList[index], dstDir + fileName));
	}
}

//------------------------------------------------------------------------------------
/**
*/
void
CEGuiExporter::CopyImagesetFiles( Util::String srcDir, Util::String dstDir )
{

	///edit the .imageset files
	Util::Array<Util::String> fileList = IO::IoServer::Instance()->ListFiles(srcDir, "*.imageset");

	if(!IO::IoServer::Instance()->DirectoryExists(dstDir))
		IO::IoServer::Instance()->CreateDirectory(dstDir);

	IndexT index;
	for(index = 0; index < fileList.Size(); index++)
	{
 		IO::URI file = IO::URI(srcDir + fileList[index]);
 		Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
 		Ptr<IO::XmlReader> xmlReader = IO::XmlReader::Create();
 
 		Util::String s;
 		s.Format("Could not open %s", file.AsString().AsCharPtr());
 		n_assert2(stream->Open(), s.AsCharPtr());
 		xmlReader->SetStream(stream);
 		n_assert(xmlReader->Open());

		//open as xml and find image file description
		xmlReader->SetToFirstChild("Imageset");
		Util::String replaceFrom = xmlReader->GetString("Imagefile");
		Util::String replaceTo = replaceFrom;
		replaceTo.StripFileExtension();
		replaceTo += ".dds";

		xmlReader->Close();

		//open / create destination file
		IO::URI exportFile = IO::URI(dstDir + fileList[index]);
		Ptr<IO::FileStream> fileStream =  IO::IoServer::Instance()->CreateStream(exportFile).downcast<IO::FileStream>();
		fileStream->SetAccessMode(IO::Stream::ReadWriteAccess);
		s.Format("Could not open %s", exportFile.AsString().AsCharPtr());
		n_assert2(fileStream->Open(), s.AsCharPtr());

		//load all data as string
		void* data = stream->Map();
		int size = stream->GetSize(); //lets hope the work file extension( tga,psd,png,dds,tar etc.) is the same size as exported file extension (dds). 3

		Util::String dataAsString((const char*)data);
		dataAsString.SubstituteString(replaceFrom, replaceTo); //replace imagefileextension to .dds

		// write to file
		fileStream->Write(dataAsString.AsCharPtr(), size);

		fileStream->Close();
		stream->Close();
	}
}

} // namespace ToolkitUtil
