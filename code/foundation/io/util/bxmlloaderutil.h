#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::BXmlLoaderUtil
    
    Helper class for loading binary xml files created with N3's
    binaryxmlconverter3 tool. BXmlLoaderUtil objects are usually
    not used directly, but are wrapped by higher level classes
    like XmlReader.

    Uses raw char pointers so that the class can both be used
    with Nebula2 and Nebula.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "io/stream.h"
#include "io/util/bxmlfilestructs.h"
#include "util/array.h"

//------------------------------------------------------------------------------
namespace IO
{
class BXmlLoaderUtil
{
public:
    static const ushort InvalidNodeIndex = 0xffff;
    static const uint InvalidAttrIndex = 0xffffffff;

    /// constructor
    BXmlLoaderUtil();
    /// destructor
    ~BXmlLoaderUtil();
    
    /// setup the object from a file-in-memory, buffer must remain intact until Discard() is called!
    void SetupFromFileInMemory(void* buf, Stream::Size size);
    /// discard the object
    void Discard();
    /// return true if the object has been setup
    bool IsValid() const;

    /// find a node index by path
    ushort FindNodeIndex(const char* path) const;
    /// set cursor to node index
    void SetCurrentNodeIndex(ushort index);
    /// get current node index
    ushort GetCurrentNodeIndex() const;
    /// get name of current node
    const char* GetCurrentNodeName() const;
    /// set cursor to first matching child node, name may point to empty string
    bool SetToFirstChild(const char* name);
    /// set cursor to next matching child node, name may point to empty string
    bool SetToNextChild(const char* name);
    /// set to parent node
    bool SetToParent();

    /// find attribute index by name
    uint FindAttrIndex(const char* attrName) const;
    /// get number of attributes on node
    uint GetNumAttrs() const;
    /// get name of attribute by index
    const char* GetAttrName(uint attrIndex) const;
    /// get value of attribute by index
    const char* GetAttrValue(uint attrIndex) const;

    /// find a child node index by name, name may point to empty string
    ushort FindChildNodeIndex(ushort nodeIndex, const char* childName) const;
    /// find a sibling node index by name, name may point to empty string
    ushort FindSiblingNodeIndex(ushort nodeIndex, const char* sibName) const;
    /// get the parent node index of a node
    ushort GetParentNodeIndex(ushort nodeIndex) const;
    /// get name of a given node
    const char* GetNodeName(ushort nodeIndex) const;

private:
    void* buffer;
    Stream::Size bufSize;

    BXMLFileHeader* header;
    BXMLFileAttr* attrs;
    BXMLFileNode* nodes;
    Util::Array<const char*> stringTable;

    ushort curNodeIndex;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
BXmlLoaderUtil::IsValid() const
{
    return (0 != this->buffer);
}

//------------------------------------------------------------------------------
/**
*/
inline void
BXmlLoaderUtil::SetCurrentNodeIndex(ushort i)
{
    n_assert((0 != this->header) && (i < this->header->numNodes));
    this->curNodeIndex = i;
}

//------------------------------------------------------------------------------
/**
*/
inline ushort
BXmlLoaderUtil::GetCurrentNodeIndex() const
{
    return this->curNodeIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline const char*
BXmlLoaderUtil::GetCurrentNodeName() const
{
    n_assert(InvalidNodeIndex != this->curNodeIndex);
    return this->stringTable[this->nodes[this->curNodeIndex].nameIndex];
}

} // namespace IO
//------------------------------------------------------------------------------
    
  