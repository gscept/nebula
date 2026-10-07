//------------------------------------------------------------------------------
//  surfaceexporter.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "surfaceexporter.h"
#include "io/xmlreader.h"
#include "io/ioserver.h"
#include "toolkitutil/converters/binaryxmlconverter.h"
#include "toolkitutil/text.h"

using namespace Util;
using namespace IO;
namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::SurfaceExporter, 'SUEX', Base::AssetProcessorBase);

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
SurfaceExporter::ProcessFile(const IO::URI& file)
{
    // get local path
    String localPath = file.LocalPath();

    // deduct file name from URL
    String fileName = localPath.ExtractFileName();
    fileName.StripFileExtension();
    Util::String category = localPath.ExtractLastDirName();
    String dst = String::Sprintf("mat:%s/%s.mat", category.AsCharPtr(), fileName.AsCharPtr());

    // create folder if it doesn't exist
    if (!IoServer::Instance()->DirectoryExists("mat:" + category))
    {
        IoServer::Instance()->CreateDirectory("mat:" + category);
    }

    // simply convert xml to binary
    this->logger->Print("%s -> %s\n", Text(file.LocalPath()).Color(TextColor::Blue).AsCharPtr(), Text(URI(Format("mat:%s/%s.mat", category.AsCharPtr(), fileName.AsCharPtr())).LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr());
    BinaryXmlConverter converter;
    converter.ConvertFile(localPath, dst, *this->logger);
}

} // namespace ToolkitUtil