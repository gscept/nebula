#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::CEGuiExporter
    
    Exports GUI
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "base/exporterbase.h"
#include "db/database.h"
#include "db/dbfactory.h"
#include "io/xmlreader.h"
#include "texutil/textureconverter.h"
#include "logger.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class CEGuiExporter : public Base::ExporterBase
{
	__DeclareClass(CEGuiExporter);

public:
	/// constructor
	CEGuiExporter();
	/// destructor
	virtual ~CEGuiExporter();

	/// opens the exporter
	void Open();
	/// closes the exporter
	void Close();
	/// export
	void ExportAll(ToolkitUtil::TextureConverter textureConverter, ToolkitUtil::Logger logger );

private:
	/// copy all existing files in directory to another directory
	void CopyFiles(Util::String srcDir, Util::String dstDir, Util::String filter = "*.*");
	/// change the .imageset files to match exported textures
	void CopyImagesetFiles(Util::String srcDir, Util::String dstDir );
};

} // namespace ToolkitUtil
//------------------------------------------------------------------------------