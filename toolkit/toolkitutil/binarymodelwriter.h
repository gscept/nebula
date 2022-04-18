#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::BinaryModelWriter
    
    Stream writer for binary model files.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "modelwriter.h"
#include "io/binarywriter.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class BinaryModelWriter : public ModelWriter
{
    __DeclareClass(BinaryModelWriter);
public:
    /// constructor
    BinaryModelWriter();
    /// destructor
    virtual ~BinaryModelWriter();

    /// begin reading from the stream
    virtual bool Open();
    /// end reading from the stream
    virtual void Close();

    /// begin writing a new model
    virtual bool BeginModel(const Util::String& className, Util::FourCC classFourCC, const Util::String& name);
    /// begin writing a model node
    virtual bool BeginModelNode(const Util::String& className, Util::FourCC classFourCC, const Util::String& name);
    /// begin a data tag
    virtual void BeginTag(const Util::String& name, Util::FourCC tagFourCC);
    /// begin writing a physics node
    virtual bool BeginPhysicsNode(const Util::String& name);
    /// begin collider node
    virtual void BeginColliderNode();
    /// write bool value
    virtual void WriteBool(bool b);
    /// write int value
    virtual void WriteInt(int i);
    /// write float value
    virtual void WriteFloat(float f);
    /// write float2 value
    virtual void WriteFloat2(const Math::vec2& f);
    /// write vec4 value
    virtual void Writevec4(const Math::vec4& f);
    /// write string value
    virtual void WriteString(const Util::String& s);
    /// write int array value
    virtual void WriteIntArray(const Util::Array<int>& a);
    /// end a data tag
    virtual void EndTag();
    /// end a collider node
    virtual void EndColliderNode();
    /// end a physics node
    virtual void EndPhysicsNode();
    /// end a model node
    virtual void EndModelNode();
    /// end model
    virtual void EndModel();

private:
    Ptr<IO::BinaryWriter> writer;
};

} // namespace ToolkitUtil
//------------------------------------------------------------------------------

