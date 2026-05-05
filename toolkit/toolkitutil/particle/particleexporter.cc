//------------------------------------------------------------------------------
//  particleexporter.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "particleexporter.h"
#include "io/xmlreader.h"
#include "io/ioserver.h"
#include "toolkit-common/converters/binaryxmlconverter.h"
#include "toolkit-common/text.h"

using namespace Util;
using namespace IO;
namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::ParticleExporter, 'PAEX', Base::ExporterBase);

//------------------------------------------------------------------------------
/**
*/
ParticleExporter::ParticleExporter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ParticleExporter::~ParticleExporter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleExporter::ExportFile(const IO::URI& file)
{
    // get local path
    String localPath = file.LocalPath();

    // deduct file name from URL
    String fileName = localPath.ExtractFileName();
    fileName.StripFileExtension();
    this->category = localPath.ExtractLastDirName();
    String dst = String::Sprintf("par:%s/%s.par", this->category.AsCharPtr(), fileName.AsCharPtr());

    // create folder if it doesn't exist
    if (!IoServer::Instance()->DirectoryExists("par:" + this->category))
    {
        IoServer::Instance()->CreateDirectory("par:" + this->category);
    }

    // simply convert xml to binary
    this->logger->Print("%s -> %s\n", Text(file.LocalPath()).Color(TextColor::Blue).AsCharPtr(), Text(URI(Format("par:%s/%s.par", this->category.AsCharPtr(), fileName.AsCharPtr())).LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr());
    IoServer::Instance()->CopyFile(localPath, dst);

    Util::String urn = Util::String::Sprintf("urn:par:%s/%s", this->category.AsCharPtr(), fileName.AsCharPtr());
    this->UpdateResourceMapping(urn, file.AsString(), IO::URI(dst).LocalPath());
}

} // namespace ToolkitUtil