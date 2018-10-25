#pragma once
//------------------------------------------------------------------------------
/**
    @file io/util/bxmlfilestructs.h
    
    Structures used by the BXML file format.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace IO
{
#pragma pack(push,1)
struct BXMLFileHeader
{
    uint magic;
    uint numAttrs;          // number of attributes in attribute array
    uint numNodes;          // number of nodes in node array
    uint numStrings;        // number of strings in string table
};
struct BXMLFileAttr
{
    ushort nameIndex;       // index into string table
    ushort valueIndex;      // index into string table
};
struct BXMLFileNode
{
    ushort nameIndex;           // index into string table
    ushort firstChildIndex;     // index to first child node
    ushort nextSiblingIndex;    // index of next sibling
    ushort parentIndex;         // index of parent
    uint attrIndex;             // index to first attr
    uint numAttrs;              // number of attributes
};
#pragma pack(pop)
}
//------------------------------------------------------------------------------
    