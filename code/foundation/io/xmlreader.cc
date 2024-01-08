//------------------------------------------------------------------------------
//  xmlreader.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "io/xmlreader.h"
#if !__OSX__
#include "math/vec4.h"
#include "math/vec2.h"
#include "math/mat4.h"
#include "math/transform44.h"
#endif

namespace IO
{
__ImplementClass(IO::XmlReader, 'XMLR', IO::StreamReader);

// This static object setsup TinyXml at application startup
// (set condense white space to false). There seems to be no easy,
// alternative way (see TinyXml docs for details)
XmlReader::TinyXmlInitHelper XmlReader::initTinyXml;

using namespace Util;
#if !__OSX__
using namespace Math;
#endif
    
//------------------------------------------------------------------------------
/**
*/
XmlReader::XmlReader() :
    xmlDocument(0),
    curNode(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
XmlReader::~XmlReader()
{
    if (this->IsOpen())
    {
        this->Close();
    }
}

//------------------------------------------------------------------------------
/**
    Opens the stream and reads the content of the stream into
    TinyXML.
*/
bool
XmlReader::Open()
{
    n_assert(0 == this->xmlDocument);
    n_assert(0 == this->curNode);

    if (StreamReader::Open())
    {
        // create an XML document object
        this->xmlDocument = new TiXmlDocument;
        if (!this->xmlDocument->LoadStream(this->stream))
        {
            // parsing XML structure failed, provide feedback
            const URI& uri = this->stream->GetURI();
            if (uri.IsValid())
            {
                n_error("XmlReader::Open(): failed to open stream as XML '%s'\nTinyXML error: %s!", uri.AsString().AsCharPtr(), this->xmlDocument->ErrorDesc());
            }
            else
            {
                n_error("XmlReader::Open(): failed to open stream as XML (URI not valid)!\nTinyXML error: %s", this->xmlDocument->ErrorDesc());
            }
            delete this->xmlDocument;
            this->xmlDocument = 0;
            return false;
        }

        // since the XML document is now loaded, we can close the original stream
        if (!this->streamWasOpen)
        {
            this->stream->Close();
        }

        // set the current node to the root node
        this->curNode = this->xmlDocument->RootElement();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
XmlReader::Close()
{
    n_assert(0 != this->xmlDocument);

    // delete the xml document
    delete this->xmlDocument;
    this->xmlDocument = 0;
    this->curNode = 0;

    StreamReader::Close();
}

//------------------------------------------------------------------------------
/**
    This method returns the line number of the current node.
*/
int
XmlReader::GetCurrentNodeLineNumber() const
{
    n_assert(this->curNode);
    return this->curNode->Row();
}

//------------------------------------------------------------------------------
/**
    This method finds an xml node by path name. It can handle absolute
    paths and paths relative to the current node. All the usual file system
    path conventions are valid: "/" is the path separator, "." is the
    current directory, ".." the parent directory.
*/
TiXmlNode*
XmlReader::FindNode(const String& path) const
{
    n_assert(this->IsOpen());
    n_assert(path.IsValid());

    bool absPath = (path[0] == '/');
    Array<String> tokens = path.Tokenize("/");

    // get starting node (either root or current node)
    TiXmlNode* node;
    if (absPath)
    {
        node = this->xmlDocument;
    }
    else
    {
        n_assert(0 != this->curNode);
        node = this->curNode;
    }

    // iterate through path components
    int i;
    int num = tokens.Size();
    for (i = 0; i < num; i++)
    {
        const String& cur = tokens[i];
        if ("." == cur)
        {
            // do nothing
        }
        else if (".." == cur)
        {
            // go to parent directory
            node = node->Parent();
            if (node == this->xmlDocument)
            {
                n_error("XmlStream::FindNode(%s): path points above root node!", path.AsCharPtr());
                return 0;
            }
        }
        else
        {
            // find child node
            node = node->FirstChild(cur.AsCharPtr());
            if (0 == node)
            {
                return 0;
            }
        }
    }
    return node;
}

//------------------------------------------------------------------------------
/**
    This method returns true if the node identified by path exists. Path
    follows the normal filesystem path conventions, "/" is the separator,
    ".." is the parent node, "." is the current node. An absolute path
    starts with a "/", a relative path doesn't.
*/
bool
XmlReader::HasNode(const String& n) const
{
    return (this->FindNode(n) != 0);
}

//------------------------------------------------------------------------------
/**
    Get the short name (without path) of the current node.
*/
String
XmlReader::GetCurrentNodeName() const
{
    n_assert(this->IsOpen());
    n_assert(0 != this->curNode);
    return String(this->curNode->Value());
}

//------------------------------------------------------------------------------
/**
    This returns the full absolute path of the current node. Path components
    are separated by slashes.
*/
String
XmlReader::GetCurrentNodePath() const
{
    n_assert(this->IsOpen());
    n_assert(0 != this->curNode);

    // build bottom-up array of node names
    Array<String> components;
    TiXmlNode* node = this->curNode;
    while (node != this->xmlDocument)
    {
        components.Append(node->Value());
        node = node->Parent();
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
    Resets the xml reader to the root node
*/
void 
XmlReader::SetToRoot()
{
    // set the current node to the root node
    this->curNode = this->xmlDocument->RootElement();

    n_assert(this->curNode);
}

//------------------------------------------------------------------------------
/**
    Set the node pointed to by the path string as current node. The path
    may be absolute or relative, following the usual filesystem path 
    conventions. Separator is a slash.
*/
void
XmlReader::SetToNode(const String& path)
{
    n_assert(this->IsOpen());
    n_assert(path.IsValid());
    TiXmlNode* n = this->FindNode(path);
    if (n)
    {
        this->curNode = n->ToElement();
    }
    else
    {
        n_error("XmlReader::SetToNode(%s): node to found!", path.AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
    Sets the current node to the first child node. If no child node exists,
    the current node will remain unchanged and the method will return false.
    If name is a valid string, only child element matching the name will 
    be returned. If name is empty, all child nodes will be considered.
*/
bool
XmlReader::SetToFirstChild(const String& name)
{
    n_assert(this->IsOpen());
    n_assert(0 != this->curNode);
    TiXmlElement* child = 0;
    if (name.IsEmpty())
    {
        child = this->curNode->FirstChildElement();
    }
    else
    {
        child = this->curNode->FirstChildElement(name.AsCharPtr());
    }
    if (child)
    {
        this->curNode = child;
        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    Sets the current node to the next sibling. If no more children exist,
    the current node will be reset to the parent node and the method will 
    return false. If name is a valid string, only child element matching the 
    name will be returned. If name is empty, all child nodes will be considered.
*/
bool
XmlReader::SetToNextChild(const String& name)
{
    n_assert(this->IsOpen());
    n_assert(0 != this->curNode);

    TiXmlElement* sib = 0;
    if (name.IsEmpty())
    {
        sib = this->curNode->NextSiblingElement();
    }
    else
    {
        sib = this->curNode->NextSiblingElement(name.AsCharPtr());
    }
    if (sib)
    {
        this->curNode = sib;
        return true;
    }
    else
    {
        this->SetToParent();
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    Sets the current node to its parent. If no parent exists, the
    current node will remain unchanged and the method will return false.
*/
bool
XmlReader::SetToParent()
{
    n_assert(this->IsOpen());
    n_assert(0 != this->curNode);
    TiXmlNode* parent = this->curNode->Parent();
    if (parent)
    {
        this->curNode = parent->ToElement();
        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    Return true if an attribute of the given name exists on the current node.
*/
bool
XmlReader::HasAttr(const char* name) const
{
    n_assert(this->IsOpen());
    n_assert(0 != this->curNode);
    n_assert(0 != name);
    return (0 != this->curNode->Attribute(name));
}

//------------------------------------------------------------------------------
/**
    Return array with names of all attrs on current node
*/
Array<String>
XmlReader::GetAttrs() const
{
    n_assert(this->IsOpen());
    n_assert(0 != this->curNode);
    Array<String> res;
    const TiXmlAttribute * attr = this->curNode->FirstAttribute();
    while(0 != attr)
    {
        res.Append(attr->Name());
        attr = attr->Next();
    }
    return res;
}

//------------------------------------------------------------------------------
/**
    Return the provided attribute as string. If the attribute does not exist
    the method will fail hard (use HasAttr() to check for its existance).
*/
String
XmlReader::GetString(const char* name) const
{
    n_assert(this->IsOpen());
    n_assert(0 != this->curNode);
    n_assert(0 != name);
    String str;
    const char* val = this->curNode->Attribute(name);    
    if (0 == val)
    {
        n_error("XmlReader: attribute '%s' doesn't exist in %s on node '%s'!", name, this->stream->GetURI().AsString().AsCharPtr(), this->curNode->Value());
    }
    else
    {
        str = val;
    }
    return str;
}

//------------------------------------------------------------------------------
/**
    Return the provided attribute as a bool. If the attribute does not exist
    the method will fail hard (use HasAttr() to check for its existance).
*/
bool
XmlReader::GetBool(const char* name) const
{
    return this->GetString(name).AsBool();
}

//------------------------------------------------------------------------------
/**
    Return the provided attribute as int. If the attribute does not exist
    the method will fail hard (use HasAttr() to check for its existance).
*/
int
XmlReader::GetInt(const char* name) const
{
    return this->GetString(name).AsInt();
}

//------------------------------------------------------------------------------
/**
    Return the provided attribute as float. If the attribute does not exist
    the method will fail hard (use HasAttr() to check for its existance).
*/
float
XmlReader::GetFloat(const char* name) const
{
    return this->GetString(name).AsFloat();
}

#if !__OSX__    
//------------------------------------------------------------------------------
/**
    Return the provided attribute as vec2. If the attribute does not exist
    the method will fail hard (use HasAttr() to check for its existance).
*/
vec2
XmlReader::GetVec2(const char* name) const
{
    return this->GetString(name).AsVec2();
}

//------------------------------------------------------------------------------
/**
    Return the provided attribute as vec4. If the attribute does not exist
    the method will fail hard (use HasAttr() to check for its existance).
*/
vec4
XmlReader::GetVec4(const char* name) const
{
#if NEBULA_XMLREADER_LEGACY_VECTORS
    const String& vec4String = this->GetString(name);
    Array<String> tokens = vec4String.Tokenize(", \t");
    if (tokens.Size() == 3)
    {
        return vec4(tokens[0].AsFloat(), tokens[1].AsFloat(), tokens[2].AsFloat(), 0);
    }    
#endif
    return this->GetString(name).AsVec4();
}

//------------------------------------------------------------------------------
/**
    Return the provided attribute as mat4. If the attribute does not exist
    the method will fail hard (use HasAttr() to check for its existance).
*/
mat4
XmlReader::GetMat4(const char* name) const
{
    return this->GetString(name).AsMat4();
}

//------------------------------------------------------------------------------
/**
Return the provided attribute as transform44. If the attribute does not exist
the method will fail hard (use HasAttr() to check for its existance).
*/
transform44
XmlReader::GetTransform44(const char* name) const
{
    return this->GetString(name).AsTransform44();
}
#endif
    
//------------------------------------------------------------------------------
/**
    Return the provided optional attribute as string. If the attribute doesn't
    exist, the default value will be returned.
*/
String
XmlReader::GetOptString(const char* name, const String& defaultValue) const
{
    if (this->HasAttr(name))
    {
        return this->GetString(name);
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
    Return the provided optional attribute as bool. If the attribute doesn't
    exist, the default value will be returned.
*/
bool
XmlReader::GetOptBool(const char* name, bool defaultValue) const
{
    if (this->HasAttr(name))
    {
        return this->GetBool(name);
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
    Return the provided optional attribute as int. If the attribute doesn't
    exist, the default value will be returned.
*/
int
XmlReader::GetOptInt(const char* name, int defaultValue) const
{
    if (this->HasAttr(name))
    {
        return this->GetInt(name);
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
    Return the provided optional attribute as float. If the attribute doesn't
    exist, the default value will be returned.
*/
float
XmlReader::GetOptFloat(const char* name, float defaultValue) const
{
    if (this->HasAttr(name))
    {
        return this->GetFloat(name);
    }
    else
    {
        return defaultValue;
    }
}

#if !__OSX__    
//------------------------------------------------------------------------------
/**
    Return the provided optional attribute as vec2. If the attribute doesn't
    exist, the default value will be returned.
*/
vec2
XmlReader::GetOptVec2(const char* name, const vec2& defaultValue) const
{
    if (this->HasAttr(name))
    {
        return this->GetVec2(name);
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
    Return the provided optional attribute as vec4. If the attribute doesn't
    exist, the default value will be returned.
*/
vec4
XmlReader::GetOptVec4(const char* name, const vec4& defaultValue) const
{
    if (this->HasAttr(name))
    {
        return this->GetVec4(name);
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
    Return the provided optional attribute as mat4. If the attribute doesn't
    exist, the default value will be returned.
*/
mat4
XmlReader::GetOptMat4(const char* name, const mat4& defaultValue) const
{
    if (this->HasAttr(name))
    {
        return this->GetMat4(name);
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
Return the provided optional attribute as transform44. If the attribute doesn't
exist, the default value will be returned.
*/
transform44
XmlReader::GetOptTransform44(const char* name, const transform44& defaultValue) const
{
    if (this->HasAttr(name))
    {
        return this->GetTransform44(name);
    }
    else
    {
        return defaultValue;
    }
}
#endif
    
//------------------------------------------------------------------------------
/**
*/
bool
XmlReader::HasContent() const
{
    n_assert(this->IsOpen());
    n_assert(0 != this->curNode);
    TiXmlNode* child = this->curNode->FirstChild();
    return child && (child->Type() == TiXmlNode::TEXT);
}

//------------------------------------------------------------------------------
/**
*/
String
XmlReader::GetContent() const
{
    n_assert(this->IsOpen());
    n_assert(this->curNode);
    TiXmlNode* child = this->curNode->FirstChild();
    n_assert(child->Type() == TiXmlNode::TEXT);
    return child->Value();
}

//------------------------------------------------------------------------------
/**
*/
bool
XmlReader::HasComment() const
{
    n_assert(this->IsOpen());
    n_assert(this->curNode);
    TiXmlNode* prevSibling = this->curNode->PreviousSibling();
    return prevSibling && (prevSibling->Type() == TiXmlNode::COMMENT);
}

//------------------------------------------------------------------------------
/**
*/
Util::String
XmlReader::GetComment() const
{
    n_assert(this->IsOpen());
    n_assert(this->curNode);
    TiXmlNode* prevSibling = this->curNode->PreviousSibling();
    n_assert(prevSibling->Type() == TiXmlNode::COMMENT);
    return prevSibling->Value();
}

} // namespace IO
