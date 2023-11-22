//------------------------------------------------------------------------------
//  surfaceexporter.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "surfaceexporter.h"
#include "io/xmlreader.h"
#include "io/ioserver.h"
#include "toolkit-common/converters/binaryxmlconverter.h"
#include "toolkit-common/text.h"

using namespace Util;
using namespace IO;
namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::SurfaceExporter, 'SUEX', Base::ExporterBase);

//------------------------------------------------------------------------------
/**
*/
SurfaceExporter::SurfaceExporter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
SurfaceExporter::~SurfaceExporter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
SurfaceExporter::ExportFile(const IO::URI& file)
{
    // get local path
    String localPath = file.LocalPath();

    // deduct file name from URL
    String fileName = localPath.ExtractFileName();
    fileName.StripFileExtension();
    String catName = localPath.ExtractLastDirName();
    String dst = String::Sprintf("sur:%s/%s.sur", catName.AsCharPtr(), fileName.AsCharPtr());

    // create folder if it doesn't exist
    if (!IoServer::Instance()->DirectoryExists("sur:" + catName))
    {
        IoServer::Instance()->CreateDirectory("sur:" + catName);
    }

    // simply convert xml to binary
    this->logger->Print("%s %s\n", "Exporting surface:", Text(Format("sur:%s/%s.sur", catName.AsCharPtr(), fileName.AsCharPtr())).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr());
    BinaryXmlConverter converter;
    converter.ConvertFile(localPath, dst, *this->logger);
}

} // namespace ToolkitUtil