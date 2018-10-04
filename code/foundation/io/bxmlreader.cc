//------------------------------------------------------------------------------
//  bxmlreader.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "bxmlreader.h"

namespace IO
{
__ImplementClass(IO::BXmlReader, 'BXLR', IO::StreamReader);

using namespace Util;
#if !__OSX__    
using namespace Math;
#endif
    
//------------------------------------------------------------------------------
/**
*/
BXmlReader::BXmlReader()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
BXmlReader::~BXmlReader()
{
    if (this->IsOpen())
    {
        this->Close();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
BXmlReader::Open()
{
    n_assert(!this->loaderUtil.IsValid());
    n_assert(this->stream->CanBeMapped());
    if (StreamReader::Open())
    {
        // map the stream content into memory
        void* buf = this->stream->Map();
        SizeT bufSize = this->stream->GetSize();

        // setup the loader util
        this->loaderUtil.SetupFromFileInMemory(buf, bufSize);
        
        return true;
    }
    // fallthrough: some error occured
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
BXmlReader::Close()
{
    n_assert(this->loaderUtil.IsValid());
    this->loaderUtil.Discard();
    this->stream->Unmap();
    StreamReader::Close();
}

//------------------------------------------------------------------------------
/**
*/
bool
BXmlReader::HasNode(const String& path) const
{
    n_assert(this->IsOpen());
    return (BXmlLoaderUtil::InvalidNodeIndex != this->loaderUtil.FindNodeIndex(path.AsCharPtr()));
}

//------------------------------------------------------------------------------
/**
*/
String
BXmlReader::GetCurrentNodeName() const
{
    n_assert(this->IsOpen());
    return String(this->loaderUtil.GetCurrentNodeName());
}

//------------------------------------------------------------------------------
/**
*/
String
BXmlReader::GetCurrentNodePath() const
{
    n_assert(this->IsOpen());

    // build bottom-up array of node names
    Array<String> components;
    ushort nodeIndex = this->loaderUtil.GetCurrentNodeIndex();
    while (BXmlLoaderUtil::InvalidNodeIndex != nodeIndex)
    {
        components.Append(this->loaderUtil.GetNodeName(nodeIndex));
        nodeIndex = this->loaderUtil.GetParentNodeIndex(nodeIndex);
    }

    // build top down path
    String path = "/";
    int i;
    for (i = components.Size() - 1; i >= 0; --i)
    {
        path.Append(components[i]);
        if (i > 0)
        {
            path.Append("/");
        }
    }
    return path;
}

//------------------------------------------------------------------------------
/**
*/
void
BXmlReader::SetToNode(const String& path)
{
    n_assert(this->IsOpen());
    ushort nodeIndex = this->loaderUtil.FindNodeIndex(path.AsCharPtr());
    if (BXmlLoaderUtil::InvalidNodeIndex == nodeIndex)
    {
        n_error("BXmlReader::SetToNode(): node not found '%s'!\n", path.AsCharPtr());
    }
    else
    {
        this->loaderUtil.SetCurrentNodeIndex(nodeIndex);
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
BXmlReader::SetToFirstChild(const String& name)
{
    n_assert(this->IsOpen());
    return this->loaderUtil.SetToFirstChild(name.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
bool
BXmlReader::SetToNextChild(const String& name)
{
    n_assert(this->IsOpen());
    return this->loaderUtil.SetToNextChild(name.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
bool
BXmlReader::SetToParent()
{
    n_assert(this->IsOpen());
    return this->loaderUtil.SetToParent();
}

//------------------------------------------------------------------------------
/**
*/
bool
BXmlReader::HasAttr(const char* attr) const
{
    n_assert(this->IsOpen());
    return (BXmlLoaderUtil::InvalidAttrIndex != this->loaderUtil.FindAttrIndex(attr));
}

//------------------------------------------------------------------------------
/**
*/
Array<String>
BXmlReader::GetAttrs() const
{
    n_assert(this->IsOpen());
    Array<String> result;
    uint numAttrs = this->loaderUtil.GetNumAttrs();
    if (numAttrs > 0)
    {
        result.Reserve(numAttrs);
        uint i;
        for (i = 0; i < numAttrs; i++)
        {
            result.Append(String(this->loaderUtil.GetAttrName(i)));
        }
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
String
BXmlReader::GetString(const char* attr) const
{
    n_assert(this->IsOpen());
    uint attrIndex = this->loaderUtil.FindAttrIndex(attr);
    if (BXmlLoaderUtil::InvalidAttrIndex == attrIndex)
    {
        n_error("BXmlReader::GetString(): attribute '%s' doesn't exist on node '%s'!", 
            attr, this->loaderUtil.GetCurrentNodeName());
        return "";
    }
    else
    {
        return String(this->loaderUtil.GetAttrValue(attrIndex));
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
BXmlReader::GetBool(const char* attr) const
{
    return this->GetString(attr).AsBool();
}

//------------------------------------------------------------------------------
/**
*/
int
BXmlReader::GetInt(const char* attr) const
{
    return this->GetString(attr).AsInt();
}

//------------------------------------------------------------------------------
/**
*/
float
BXmlReader::GetFloat(const char* attr) const
{
    return this->GetString(attr).AsFloat();
}

#if !__OSX__
//------------------------------------------------------------------------------
/**
*/
Math::float2
BXmlReader::GetFloat2(const char* attr) const
{
    return this->GetString(attr).AsFloat2();
}

//------------------------------------------------------------------------------
/**
*/
Math::float4
BXmlReader::GetFloat4(const char* attr) const
{
#if NEBULA_XMLREADER_LEGACY_VECTORS
    const String& float4String = this->GetString(attr);
    Array<String> tokens = float4String.Tokenize(", \t");
    if (tokens.Size() == 3)
    {
        return float4(tokens[0].AsFloat(), tokens[1].AsFloat(), tokens[2].AsFloat(), 0);
    }    
#endif
    return this->GetString(attr).AsFloat4();
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
BXmlReader::GetMatrix44(const char* attr) const
{
    return this->GetString(attr).AsMatrix44();
}
#endif
    
//------------------------------------------------------------------------------
/**
*/
String
BXmlReader::GetOptString(const char* attr, const String& defaultValue) const
{
    if (this->HasAttr(attr))
    {
        return this->GetString(attr);
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
BXmlReader::GetOptBool(const char* attr, bool defaultValue) const
{
    if (this->HasAttr(attr))
    {
		String str = this->GetString(attr);
		if (str.IsValidBool())  return str.AsBool();
		else					return defaultValue;
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
*/
int
BXmlReader::GetOptInt(const char* attr, int defaultValue) const
{
    if (this->HasAttr(attr))
    {
		String str = this->GetString(attr);
		if (str.IsValidInt())   return str.AsInt();
		else					return defaultValue;
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
*/
float
BXmlReader::GetOptFloat(const char* attr, float defaultValue) const
{
    if (this->HasAttr(attr))
    {
		String str = this->GetString(attr);
		if (str.IsValidFloat()) return str.AsFloat();
		else					return defaultValue;
    }
    else
    {
        return defaultValue;
    }
}

#if !__OSX__    
//------------------------------------------------------------------------------
/**
*/
Math::float2
BXmlReader::GetOptFloat2(const char* attr, const Math::float2& defaultValue) const
{
    if (this->HasAttr(attr))
    {
		String str = this->GetString(attr);
		if (str.IsValidFloat2()) return str.AsFloat2();
		else					 return defaultValue;
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
*/
Math::float4
BXmlReader::GetOptFloat4(const char* attr, const Math::float4& defaultValue) const
{
    if (this->HasAttr(attr))
    {
		String str = this->GetString(attr);
		if (str.IsValidFloat4()) return str.AsFloat4();
		else					 return defaultValue;
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
BXmlReader::GetOptMatrix44(const char* attr, const Math::matrix44& defaultValue) const
{
    if (this->HasAttr(attr))
    {
		String str = this->GetString(attr);
		if (str.IsValidMatrix44())  return str.AsMatrix44();
		else						return defaultValue;
    }
    else
    {
        return defaultValue;
    }
}
#endif
    
} // namespace IO
