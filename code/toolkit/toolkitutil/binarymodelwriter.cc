//------------------------------------------------------------------------------
//  binarymodelwriter.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "binarymodelwriter.h"
#include "system/byteorder.h"

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::BinaryModelWriter, 'BMWR', ToolkitUtil::ModelWriter);

using namespace Math;
using namespace Util;
using namespace IO;
using namespace System;

//------------------------------------------------------------------------------
/**
*/
BinaryModelWriter::BinaryModelWriter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
BinaryModelWriter::~BinaryModelWriter()
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
BinaryModelWriter::Open()
{
    n_assert(!this->isOpen);
    n_assert(this->stream.isvalid());
    n_assert(this->stream->CanWrite());

    if (StreamWriter::Open())
    {
        // setup a BinaryWriter
        this->writer = BinaryWriter::Create();
        this->writer->SetStream(this->stream);
        if (Platform::Win32 == this->platform)
        {
            this->writer->SetStreamByteOrder(ByteOrder::LittleEndian);
        }
        else
        {
            this->writer->SetStreamByteOrder(ByteOrder::BigEndian);
        }

        if (this->writer->Open())
        {
            // write header
            this->writer->WriteUInt(FourCC('NEB3').AsUInt());
            this->writer->WriteInt(this->version);
            return true;
        }

    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryModelWriter::Close()
{
    n_assert(this->isOpen);

    this->writer->WriteUInt(FourCC('EOF_').AsUInt());
    this->writer->Close();
    this->writer = 0;

    StreamWriter::Close();
}

//------------------------------------------------------------------------------
/**
*/
bool
BinaryModelWriter::BeginModel(const String& className, FourCC classFourCC, const String& name)
{
    n_assert(classFourCC.IsValid());
    n_assert(name.IsValid());
    this->writer->WriteUInt(FourCC('>MDL').AsUInt());
    this->writer->WriteUInt(classFourCC.AsUInt());
    this->writer->WriteString(name);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryModelWriter::EndModel()
{
    this->writer->WriteUInt(FourCC('<MDL').AsUInt());
}

//------------------------------------------------------------------------------
/**
*/
bool
BinaryModelWriter::BeginModelNode(const String& className, FourCC classFourCC, const String& name)
{
    n_assert(classFourCC.IsValid());
    n_assert(name.IsValid());
    this->writer->WriteUInt(FourCC('>MND').AsUInt());
    this->writer->WriteUInt(classFourCC.AsUInt());
    this->writer->WriteString(name);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
BinaryModelWriter::BeginPhysicsNode(const Util::String& name)
{
	this->writer->WriteUInt(FourCC('>PHN').AsUInt());
	this->writer->WriteString(name);
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryModelWriter::BeginColliderNode()
{
	this->writer->WriteUInt(FourCC('>CLR').AsUInt());
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryModelWriter::EndPhysicsNode()
{
	this->writer->WriteUInt(FourCC('<PHN').AsUInt());
}


//------------------------------------------------------------------------------
/**
*/
void
BinaryModelWriter::EndModelNode()
{
    this->writer->WriteUInt(FourCC('<MND').AsUInt());
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryModelWriter::BeginTag(const String& tagName, FourCC tagFourCC)
{
    n_assert(tagFourCC.IsValid());
    this->writer->WriteUInt(tagFourCC.AsUInt());
}
//------------------------------------------------------------------------------
/**
*/
void
BinaryModelWriter::EndTag()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryModelWriter::EndColliderNode()
{
	this->writer->WriteUInt(FourCC('<CLR').AsUInt());
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryModelWriter::WriteInt(int i)
{
    this->writer->WriteInt(i);
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryModelWriter::WriteFloat(float f)
{
    this->writer->WriteFloat(f);
}

//------------------------------------------------------------------------------
/**
*/
void 
BinaryModelWriter::WriteFloat2( const Math::float2& f )
{
	this->writer->WriteFloat2(f);
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryModelWriter::WriteFloat4(const float4& f4)
{
    this->writer->WriteFloat4(f4);
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryModelWriter::WriteString(const String& s)
{
    this->writer->WriteString(s);
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryModelWriter::WriteIntArray(const Array<int>& a)
{
    this->writer->WriteInt(a.Size());
    IndexT i;
    for (i = 0; i < a.Size(); i++)
    {
        this->writer->WriteInt(a[i]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
BinaryModelWriter::WriteBool(bool b)
{
    this->writer->WriteBool(b);
}
} // namespace ToolkitUtil