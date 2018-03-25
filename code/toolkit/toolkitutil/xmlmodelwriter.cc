//------------------------------------------------------------------------------
//  xmlmodelwriter.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "xmlmodelwriter.h"
#include "math/float4.h"

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::XmlModelWriter, 'XMWR', ToolkitUtil::ModelWriter);

using namespace Util;
using namespace IO;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
XmlModelWriter::XmlModelWriter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
XmlModelWriter::~XmlModelWriter()
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
XmlModelWriter::Open()
{
    n_assert(!this->isOpen);
    n_assert(this->stream.isvalid());
    n_assert(this->stream->CanWrite());

    if (StreamWriter::Open())
    {
        // setup an XmlWriter
        this->writer = XmlWriter::Create();
        this->writer->SetStream(this->stream);
        if (this->writer->Open())
        {
            // write root element
            this->writer->BeginNode("Nebula3Model");
            this->writer->SetInt("version", this->version);
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
XmlModelWriter::Close()
{
    n_assert(this->isOpen);

    this->writer->EndNode();
    this->writer->Close();
    this->writer = nullptr;

    StreamWriter::Close();
}

//------------------------------------------------------------------------------
/**
*/
bool
XmlModelWriter::BeginModel(const String& className, FourCC classFourCC, const String& name)
{
    n_assert(className.IsValid());
    n_assert(name.IsValid());
    this->writer->BeginNode("Model");
    this->writer->SetString("className", className);
    this->writer->SetString("name", name);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
XmlModelWriter::EndModel()
{
    this->writer->EndNode();
}

//------------------------------------------------------------------------------
/**
*/
bool
XmlModelWriter::BeginModelNode(const String& className, FourCC classFourCC, const String& name)
{
    n_assert(className.IsValid());
    n_assert(name.IsValid());
    this->writer->BeginNode("ModelNode");
    this->writer->SetString("className", className);
    this->writer->SetString("name", name);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
XmlModelWriter::BeginPhysicsNode(const String& name)
{
	this->writer->BeginNode("PhysicsNode");
	this->writer->SetString("name",name);
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void
XmlModelWriter::BeginColliderNode()
{
	this->writer->BeginNode("ColliderNode");
}

//------------------------------------------------------------------------------
/**
*/
void
XmlModelWriter::EndPhysicsNode()
{
	this->writer->EndNode();
}


//------------------------------------------------------------------------------
/**
*/
void
XmlModelWriter::EndModelNode()
{
    this->writer->EndNode();
}

//------------------------------------------------------------------------------
/**
*/
void
XmlModelWriter::BeginTag(const String& tagName, FourCC tagFourCC)
{
    n_assert(tagName.IsValid());
    this->writer->WriteComment(tagName);
    this->writer->BeginNode(tagFourCC.AsString());
}

//------------------------------------------------------------------------------
/**
*/
void
XmlModelWriter::EndTag()
{
    this->writer->EndNode();
}

//------------------------------------------------------------------------------
/**
*/
void
XmlModelWriter::EndColliderNode()
{
	this->writer->EndNode();
}

//------------------------------------------------------------------------------
/**
*/
void
XmlModelWriter::WriteInt(int i)
{
    this->writer->BeginNode("i");
    this->writer->WriteContent(String::FromInt(i));
    this->writer->EndNode();
}

//------------------------------------------------------------------------------
/**
*/
void
XmlModelWriter::WriteFloat(float f)
{
    this->writer->BeginNode("f");
    this->writer->WriteContent(String::FromFloat(f));
    this->writer->EndNode();
}

//------------------------------------------------------------------------------
/**
*/
void 
XmlModelWriter::WriteFloat2( const Math::float2& f )
{
	this->writer->BeginNode("f2");
	this->writer->WriteContent(String::FromFloat2(f));
	this->writer->EndNode();
}

//------------------------------------------------------------------------------
/**
*/
void
XmlModelWriter::WriteFloat4(const float4& f4)
{
    this->writer->BeginNode("f4");
    this->writer->WriteContent(String::FromFloat4(f4));
    this->writer->EndNode();
}

//------------------------------------------------------------------------------
/**
*/
void
XmlModelWriter::WriteString(const String& s)
{
    this->writer->BeginNode("s");
    this->writer->WriteContent(s);
    this->writer->EndNode();
}

//------------------------------------------------------------------------------
/**
*/
void
XmlModelWriter::WriteIntArray(const Array<int>& a)
{
    this->writer->BeginNode("ias");
    this->writer->WriteContent(String::FromInt(a.Size()));
    this->writer->EndNode();
    String str;
    IndexT i;
    for (i = 0; i < a.Size(); i++)
    {
        str.Append(String::FromInt(a[i]));
        if (i < (a.Size() - 1))
        {
            str.Append(",");
        }
    }
    this->writer->BeginNode("ia");
    this->writer->WriteContent(str);
    this->writer->EndNode();
}

//------------------------------------------------------------------------------
/**
*/
void 
XmlModelWriter::WriteBool(bool b)
{
    this->writer->BeginNode("b");
    this->writer->WriteContent(String::FromBool(b));
    this->writer->EndNode();
}
} // namespace ToolkitUtil