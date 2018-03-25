//------------------------------------------------------------------------------
//  binaryxmlconverter.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "binaryxmlconverter.h"
#include "io/ioserver.h"
#include "io/binarywriter.h"

namespace ToolkitUtil
{
using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
BinaryXmlConverter::BinaryXmlConverter() :
    platform(Platform::Win32)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
BinaryXmlConverter::~BinaryXmlConverter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Toplevel conversion routine, recursively convert XML files from
    source directory to destination directory.
*/
bool
BinaryXmlConverter::ConvertDir(const Util::String& srcDir, const Util::String& dstDir, Logger& logger)
{
    IoServer* ioServer = IoServer::Instance();
	this->srcDir = srcDir;
	this->dstDir = dstDir;

    // make sure directories are valid
    if (!this->srcDir.IsValid())
    {
        logger.Error("No source directory set!\n");
        return false;
    }
    if (!this->dstDir.IsValid())
    {
        logger.Error("No destination directory set!\n");
        return false;
    }
    if (!ioServer->DirectoryExists(this->srcDir))
    {
        logger.Error("Source directory '%s' does not exist!\n", this->srcDir.AsCharPtr());
        return false;
    }

    // recursively convert XML files
    if (!this->RecurseConvert(this->srcDir, this->dstDir, logger))
    {
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
BinaryXmlConverter::RecurseConvert(const String& sourceDir, const String& destDir, Logger& logger)
{
    IoServer* ioServer = IoServer::Instance();

    // make sure the destination directory exists
    if (!ioServer->DirectoryExists(destDir))
    {
        if (!ioServer->CreateDirectory(destDir))
        {
            logger.Error("Failed to create destination directory '%s'!\n", destDir.AsCharPtr());
            return false;
        }
    }

    // for each XML file...
    Array<String> srcFiles = ioServer->ListFiles(sourceDir, "*.xml");
    IndexT i;
    for (i = 0; i < srcFiles.Size(); i++)
    {
        String srcFile = sourceDir + "/" + srcFiles[i];
        String dstFile = destDir + "/" + srcFiles[i];
        dstFile.StripFileExtension();
        dstFile.Append(".bxml");
        if (!this->ConvertFile(srcFile, dstFile, logger))
        {
            // continue even if one file failed to convert
            logger.Error("FAILED TO CONVERT '%s'!!!\n", srcFile.AsCharPtr());
        }
    }

    // recurse into subdirectory
    Array<String> dirs = ioServer->ListDirectories(sourceDir, "*");
    for (i = 0; i < dirs.Size(); i++)
    {
        if ((dirs[i] != ".svn") && (dirs[i] != "CVS"))
        {
            String srcSubDir = sourceDir + "/" + dirs[i];
            String dstSubDir = destDir + "/" + dirs[i];
            if (!this->RecurseConvert(srcSubDir, dstSubDir, logger))
            {
                return false;
            }
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
BinaryXmlConverter::ConvertFile(const String& srcFile, const String& dstFile, Logger& logger)
{
    logger.Print("> %s -> %s\n", srcFile.AsCharPtr(), dstFile.AsCharPtr());
    
    if (!this->LoadFile(srcFile, logger))
    {   
        return false;
    }
    if (!this->ValidateInternalRepresentation(srcFile, logger))
    {
        return false;
    }
    this->BuildFileArrays();
    if (!this->SaveFile(dstFile, logger))
    {
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryXmlConverter::ClearUniqueStrings()
{
    this->stringTable.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryXmlConverter::AddUniqueString(const StringAtom& str)
{
    if (InvalidIndex == this->stringTable.BinarySearchIndex(str))
    {
        this->stringTable.InsertSorted(str);
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
BinaryXmlConverter::LoadFile(const String& srcFile, Logger& logger)
{
    // clear the unique-string table
    this->ClearUniqueStrings();

    // open source file as XML
    IoServer* ioServer = IoServer::Instance();
    Ptr<Stream> stream = ioServer->CreateStream(srcFile);
    Ptr<XmlReader> xmlReader = XmlReader::Create();
    xmlReader->SetStream(stream);
    if (!xmlReader->Open())
    {
        logger.Error("Failed to open source file as XML: '%s'\n", srcFile.AsCharPtr());
        return false;
    }

    // recursively parse XML content
    if (!this->RecurseParseXml(xmlReader, this->rootNode, logger))
    {
        logger.Error("Error parsing XML file '%s'!\n", srcFile.AsCharPtr());
        return false;
    }

    // success
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
BinaryXmlConverter::RecurseParseXml(const Ptr<XmlReader>& xmlReader, Node& node, Logger& logger)
{
    // parse name of node
    node.name = xmlReader->GetCurrentNodeName();
    node.attrs.Clear();
    node.childNodes.Clear();
    this->AddUniqueString(node.name);

    // parse node attributes
    Array<String> attrs = xmlReader->GetAttrs();
    if (attrs.Size() > 0)
    {
        IndexT attrIndex;
        node.attrs.Reserve(attrs.Size());
        for (attrIndex = 0; attrIndex < attrs.Size(); attrIndex++)
        {
            Attr attr;
            attr.name  = attrs[attrIndex];
            attr.value = xmlReader->GetString(attrs[attrIndex].AsCharPtr());
            this->AddUniqueString(attr.name);
            this->AddUniqueString(attr.value);
            node.attrs.Append(attr);
        }
    }

    // parse child nodes
    if (xmlReader->SetToFirstChild()) do
    {
        Node newNode;
        node.childNodes.Append(newNode);
        if (!this->RecurseParseXml(xmlReader, node.childNodes.Back(), logger))
        {
            return false;
        }
    }
    while (xmlReader->SetToNextChild());

    // done
    return true;
}

//------------------------------------------------------------------------------
/**
*/
SizeT
BinaryXmlConverter::RecurseCountAttrs(const Node& node, SizeT numAttrs)
{
    numAttrs += node.attrs.Size();
    IndexT i;
    for (i = 0; i < node.childNodes.Size(); i++)
    {
        numAttrs = this->RecurseCountAttrs(node.childNodes[i], numAttrs);
    }
    return numAttrs;
}

//------------------------------------------------------------------------------
/**
*/
SizeT
BinaryXmlConverter::RecurseCountNodes(const Node& node, SizeT numNodes)
{
    numNodes += node.childNodes.Size();
    IndexT i;
    for (i = 0; i < node.childNodes.Size(); i++)
    {
        numNodes = this->RecurseCountNodes(node.childNodes[i], numNodes);
    }
    return numNodes;
}

//------------------------------------------------------------------------------
/**
*/
bool
BinaryXmlConverter::ValidateInternalRepresentation(const String& srcFile, Logger& logger)
{
    if (this->stringTable.Size() >= 65536)
    {
        logger.Error("Too many unique strings (>=65536) in '%s'!\n", srcFile.AsCharPtr());
        return false;
    }
    SizeT numNodes = this->RecurseCountNodes(this->rootNode, 0);
    if (numNodes >= 65535)
    {
        logger.Error("Too many XML nodes in '%s' (must be < 65535)!\n", srcFile.AsCharPtr());
        return false;
    }
    SizeT numAttrs = this->RecurseCountAttrs(this->rootNode, 0);
    logger.Print("%d unique strings, %d nodes, %d attrs\n", this->stringTable.Size(), numNodes, numAttrs);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryXmlConverter::BuildFileArrays()
{
    this->fileAttrArray.Clear();
    this->fileNodeArray.Clear();
    this->RecurseAddFileNode(this->rootNode, InvalidNodeIndex, InvalidNodeIndex);
}

//------------------------------------------------------------------------------
/**
*/
ushort
BinaryXmlConverter::RecurseAddFileNode(const Node& node, ushort prevSiblingIndex, ushort parentNodeIndex)
{
    // build a new BXMLNode and append to node array
    BXMLFileNode bxmlNode;
    bxmlNode.nameIndex = this->stringTable.BinarySearchIndex(node.name);
    bxmlNode.firstChildIndex = InvalidNodeIndex;
    bxmlNode.nextSiblingIndex = InvalidNodeIndex;
    bxmlNode.parentIndex = parentNodeIndex;
    if (node.attrs.Size() > 0)
    {
        bxmlNode.attrIndex = this->fileAttrArray.Size();
        bxmlNode.numAttrs = node.attrs.Size();
    }
    else
    {
        bxmlNode.attrIndex = InvalidAttrIndex;
        bxmlNode.numAttrs = 0;
    }
    ushort myNodeIndex = this->fileNodeArray.Size();
    this->fileNodeArray.Append(bxmlNode);

    // patch dependend nodes (parent or prev sibling
    if (prevSiblingIndex != InvalidNodeIndex)
    {
        this->fileNodeArray[prevSiblingIndex].nextSiblingIndex = myNodeIndex;
    }
    else if (parentNodeIndex != InvalidNodeIndex)
    {
        this->fileNodeArray[parentNodeIndex].firstChildIndex = myNodeIndex;
    }

    // add attributes
    IndexT attrIndex;
    for (attrIndex = 0; attrIndex < node.attrs.Size(); attrIndex++)
    {
        BXMLFileAttr bxmlAttr;
        bxmlAttr.nameIndex = this->stringTable.BinarySearchIndex(node.attrs[attrIndex].name);
        bxmlAttr.valueIndex = this->stringTable.BinarySearchIndex(node.attrs[attrIndex].value);
        this->fileAttrArray.Append(bxmlAttr);
    }

    // recurse into child nodes...
    ushort prevChildIndex = InvalidNodeIndex;
    IndexT childIndex;
    for (childIndex = 0; childIndex < node.childNodes.Size(); childIndex++)
    {
        prevChildIndex = this->RecurseAddFileNode(node.childNodes[childIndex], prevChildIndex, myNodeIndex);
    }

    // return my own node index
    return myNodeIndex;
}
 
//------------------------------------------------------------------------------
/**
*/
bool
BinaryXmlConverter::SaveFile(const String& dstFile, Logger& logger)
{
    Ptr<Stream> stream = IoServer::Instance()->CreateStream(dstFile);
    Ptr<BinaryWriter> writer = BinaryWriter::Create();
    writer->SetStream(stream);
    writer->SetStreamByteOrder(Platform::GetPlatformByteOrder(this->platform));
    if (writer->Open())
    {
        // write header
        BXMLFileHeader header = { 0 };
        header.magic = 'BXML';
        header.numAttrs = this->fileAttrArray.Size();
        header.numNodes = this->fileNodeArray.Size();
        header.numStrings = this->stringTable.Size();
        writer->WriteUInt(header.magic);
        writer->WriteUInt(header.numAttrs);
        writer->WriteUInt(header.numNodes);
        writer->WriteUInt(header.numStrings);

        // write attribute array
        IndexT attrIndex;
        for (attrIndex = 0; attrIndex < this->fileAttrArray.Size(); attrIndex++)
        {
            const BXMLFileAttr& attr = this->fileAttrArray[attrIndex];
            writer->WriteUShort(attr.nameIndex);
            writer->WriteUShort(attr.valueIndex);
        }

        // write node array
        IndexT nodeIndex;
        for (nodeIndex = 0; nodeIndex < this->fileNodeArray.Size(); nodeIndex++)
        {
            const BXMLFileNode& node = this->fileNodeArray[nodeIndex];
            writer->WriteUShort(node.nameIndex);
            writer->WriteUShort(node.firstChildIndex);
            writer->WriteUShort(node.nextSiblingIndex);
            writer->WriteUShort(node.parentIndex);
            writer->WriteUInt(node.attrIndex);
            writer->WriteUInt(node.numAttrs);
        }

        // finally, write the string table
        IndexT strIndex;
        for (strIndex = 0; strIndex < this->stringTable.Size(); strIndex++)
        {
            const char* strPtr = this->stringTable[strIndex].Value();
            writer->WriteRawData(strPtr, String::StrLen(strPtr) + 1);
        }

        // done!
        writer->Close();
    }
    return true;
}

} // namespace ToolkitUtil