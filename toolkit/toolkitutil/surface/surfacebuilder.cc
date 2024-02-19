//------------------------------------------------------------------------------
//  surfacebuilder.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "surfacebuilder.h"
#include "io/ioserver.h"
#include "toolkit-common/converters/binaryxmlconverter.h"
#include "io/xmlwriter.h"
#include "io/memorystream.h"
#include "toolkit-common/text.h"

using namespace IO;
using namespace Util;

namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
*/
SurfaceBuilder::SurfaceBuilder()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
SurfaceBuilder::~SurfaceBuilder()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
SurfaceBuilder::ExportBinary(Util::String const& dstFile)
{
    // create folder if it doesn't exist
    if (!IoServer::Instance()->DirectoryExists(this->dstDir))
    {
        IoServer::Instance()->CreateDirectory(this->dstDir);
    }

    Ptr<MemoryStream> stream = MemoryStream::Create();
    stream->SetAccessMode(Stream::AccessMode::WriteAccess);
    
    Ptr<XmlWriter> writer = XmlWriter::Create();
    writer->SetStream(stream);
    writer->Open();

    writer->BeginNode("Nebula");
    {
        writer->BeginNode("Surface");
            writer->SetString("template", this->material);
            {
                for (IndexT i = 0; i < this->params.Size(); i++)
                {
                    writer->BeginNode("Param");
                    writer->SetString("name", this->params[i].Key());
                    writer->SetString("value", this->params[i].Value());
                    writer->EndNode();
                }
            }
            writer->BeginNode("Params");
            for (IndexT i = 0; i < this->params.Size(); i++)
            {
                writer->BeginNode(this->params[i].Key());
                writer->SetString("value", this->params[i].Value());
                writer->EndNode();
            }
            writer->EndNode();
        writer->EndNode();
    }
    writer->EndNode();
    writer->Close();

    stream->SetAccessMode(Stream::AccessMode::ReadAccess);

    this->logger->Print("Generated surface: %s\n", Text(dstFile).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr());
    BinaryXmlConverter converter;
    converter.ConvertStream(stream, dstFile, *this->logger);
}

} // namespace ToolkitUtil
