#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::N3XMLConverter
    
    Converts an XML file to binary .n3
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "io/xmlreader.h"
#include "toolkitutil/binarymodelwriter.h"
#include "io/uri.h"
#include "toolkit-common/base/exporterbase.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class N3XmlExporter : public Base::ExporterBase
{
    __DeclareClass(N3XmlExporter);

public:
    /// constructor
    N3XmlExporter();
    /// destructor
    virtual ~N3XmlExporter();

    /// opens the exporter
    void Open();
    /// closes the exporter
    void Close();

    /// exports single file
    void ExportFile(const IO::URI& file);
    /// exports directory
    void ExportDir(const Util::String& category);
    /// exports all
    void ExportAll();

    /// exports from memory
    void ExportMemory(const Util::String& file, const IO::URI& dest);
private:
    
    /// recursively parses through node and its children
    bool RecursiveParse(Ptr<IO::XmlReader> reader, Ptr<BinaryModelWriter> writer);

    Ptr<IO::XmlReader> modelReader;
    Ptr<BinaryModelWriter> modelWriter;
}; 

} // namespace ToolkitUtil
//------------------------------------------------------------------------------