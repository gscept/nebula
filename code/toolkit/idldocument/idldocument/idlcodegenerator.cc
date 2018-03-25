//------------------------------------------------------------------------------
//  idlcodegenerator.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "idlcodegenerator.h"
#include "io/ioserver.h"

namespace Tools
{
__ImplementClass(Tools::IDLCodeGenerator, 'IDCG', Core::RefCounted);

using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
IDLCodeGenerator::IDLCodeGenerator()
    :hasError(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
URI
IDLCodeGenerator::BuildHeaderUri() const
{
	Util::String str;
	if(!this->outputUri.IsEmpty())
	{

		// we need to append the local path components of the input uri to the output folder and then create them as well
		URI newdir = outputUri;

		str = this->uri.LocalPath();	
				
		// ExtractDirName will return the whole filename if no folder is present (bug?) workaround
		if(str.FindCharIndex('/') != InvalidIndex)
		{
			Util::String dir = str.ExtractDirName();		
			newdir.AppendLocalPath(dir);		
		}
		// check that directories exist
		IO::IoServer::Instance()->CreateDirectory(newdir);

		// now construct the actual full path
		Util::String file = str.ExtractFileName();
		str = newdir.AsString();
		str.Append("/");
		str.Append(file);		
	}
	else
	{
		str = this->uri.AsString();
	}     
    str.StripFileExtension();
    str.Append(".h");
    URI uri(str);
    return uri;
}

//------------------------------------------------------------------------------
/**
*/
URI
IDLCodeGenerator::BuildSourceUri() const
{
	Util::String str;
	if(!this->outputUri.IsEmpty())
	{
		// we need to append the local path components of the input uri to the output folder and then create them as well
		URI newdir = outputUri;

		str = this->uri.LocalPath();	

		// ExtractDirName will return the whole filename if no folder is present (bug?) workaround
		if(str.FindCharIndex('/') != InvalidIndex)
		{
			Util::String dir = str.ExtractDirName();		
			newdir.AppendLocalPath(dir);		
		}
		// check that directories exist
		IO::IoServer::Instance()->CreateDirectory(newdir);

		// now construct the actual full path
		Util::String file = str.ExtractFileName();
		str = newdir.AsString();
		str.Append("/");
		str.Append(file);		
	}
	else
	{
		str = this->uri.AsString();
	}     
	str.StripFileExtension();
    str.Append(".cc");
    URI uri(str);
    return uri;
}

//------------------------------------------------------------------------------
/**
*/
void
IDLCodeGenerator::SetError(const char* fmt, ...)
{
    this->hasError = true;
    Util::String err;
    va_list argList;
    va_start(argList, fmt);
    err.FormatArgList(fmt, argList);
    va_end(argList);
    this->error.Append(err);
}

//------------------------------------------------------------------------------
/**
*/
bool
IDLCodeGenerator::GenerateIncludeFile()
{
    n_assert(this->uri.IsValid());
    n_assert(this->document.isvalid());
    
    // open header file
    URI uri = this->BuildHeaderUri();
    Ptr<Stream> stream = IoServer::Instance()->CreateStream(uri);
    stream->SetAccessMode(Stream::WriteAccess);
    // FIXME. do we need this
    Util::String folder = uri.LocalPath().ExtractDirName();
    URI furi(folder);
    furi.SetScheme("file");    
    if(!IoServer::Instance()->DirectoryExists(folder))
    {
        IoServer::Instance()->CreateDirectory(furi);
    } 
    if (stream->Open())
    {
        Ptr<TextWriter> writer = TextWriter::Create();
        writer->SetStream(stream);
        if (writer->Open())
        {
            this->WriteIncludeHeader(writer);
            this->WriteAttributeLibraryDeclaration(writer);
            this->WriteLibraryDeclarations(writer);
            if (!this->WriteProtocolDeclarations(writer))
            {
                return false;
            }
            this->WriteIncludeFooter(writer);
            writer->Close();
        }
        else
        {
            this->SetError("Could not open text writer on stream '%s'!\n", uri.AsString().AsCharPtr());
            return false;
        }
        stream->Close();
    }
    else
    {
        this->SetError("Could not open stream '%s' for writing!\n", uri.AsString().AsCharPtr());
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
IDLCodeGenerator::GenerateSourceFile()
{
    n_assert(this->uri.IsValid());
    n_assert(this->document.isvalid());

    // open source file
    URI uri = this->BuildSourceUri();
    Ptr<Stream> stream = IoServer::Instance()->CreateStream(uri);
    stream->SetAccessMode(Stream::WriteAccess);
    if (stream->Open())
    {
        Ptr<TextWriter> writer = TextWriter::Create();
        writer->SetStream(stream);
        if (writer->Open())
        {
            this->WriteSourceHeader(writer);
            this->WriteLibraryImplementations(writer);
            this->WriteSourceFooter(writer);
            writer->Close();
        }
        else
        {
            this->SetError("Could not open text writer on stream '%s'!\n", uri.AsString().AsCharPtr());
            return false;
        }
        stream->Close();
    }
    else
    {
        this->SetError("Could not open stream '%s' for writing!\n", uri.AsString().AsCharPtr());
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Converts a IDL type string into a Nebula C++ reference type.
    OBSOLETE??? -> old math types?
*/
String
IDLCodeGenerator::GetNebulaRefType(const String& type) 
{
    if ("string" == type)               return "const Util::String&";    
    else if ("float4" == type)          return "const Math::float4&";
    else if ("matrix44" == type)        return "const Math::matrix44&";
    else if ("transform44" == type)     return "const Math::transform44&";
    else if ("bool" == type)            return "bool";
    else if ("guid" == type)            return "const Util::Guid&";
    else if ("int" == type)             return "int";
    else if ("uint" == type)            return "uint";
    else if ("float" == type)           return "float";
    else if ("object" == type)          return "Core::RefCounted*";
    else if ("voidptr" == type)         return "void*";
    else if ("intArray" == type)        return "const Util::Array<int>&";
    else if ("floatArray" == type)      return "const Util::Array<float>&";
    else if ("boolArray" == type)       return "const Util::Array<bool>&";
    else if ("stringArray" == type)     return "const Util::Array<Util::String>&";
    else if ("vectorArray" == type)     return "const Util::Array<Math::vector>&";
    else if ("float4Array" == type)     return "const Util::Array<Math::float4>&";
    else if ("matrix44Array" == type)   return "const Util::Array<Math::matrix44>&";
    else if ("guidArray" == type)       return "const Util::Array<Util::Guid>&";
    else if ("Ptr<Game::Entity>" == type) return "const Ptr<Game::Entity>&";
    else
    {
        this->SetError("IDLCodeGenerator::GetNebulaRefType(): Invalid IDL type '%s'!", type.AsCharPtr());
        return "";
    }
}

//------------------------------------------------------------------------------
/**
    Converts a IDL type string into a Nebula C++ type.
    OBSOLETE??? -> old math types?
*/
String
IDLCodeGenerator::GetNebulaType(const String& type) 
{
    if ("string" == type)               return "Util::String";    
    else if ("float4" == type)          return "Math::float4";
    else if ("matrix44" == type)        return "Math::matrix44";
    else if ("transform44" == type)     return "Math::transform44";
    else if ("bool" == type)            return "bool";
    else if ("guid" == type)            return "Util::Guid";
    else if ("int" == type)             return "int";
    else if ("uint" == type)            return "uint";
    else if ("float" == type)           return "float";
    else if ("object" == type)          return "Core::RefCounted*";
    else if ("voidptr" == type)         return "void*";
    else if ("intArray" == type)        return "Util::Array<int>";
    else if ("floatArray" == type)      return "Util::Array<float>";
    else if ("boolArray" == type)       return "Util::Array<bool>";
    else if ("stringArray" == type)     return "Util::Array<Util::String>";
    else if ("vectorArray" == type)     return "Util::Array<Math::vector>";
    else if ("float4Array" == type)     return "Util::Array<Math::float4>";
    else if ("matrix44Array" == type)   return "Util::Array<Math::matrix44>";
    else if ("guidArray" == type)       return "Util::Array<Util::Guid>";
    else if ("GameEntity" == type)      return "Ptr<Game::Entity>";
    else
    {
        this->SetError("IDLCodeGenerator::GetNebulaType(): Invalid IDL type '%s'!", type.AsCharPtr());
        return "";
    }
}

//------------------------------------------------------------------------------
/**
    Converts a IDL type string into a Nebula C++ type.
    OBSOLETE??? -> old math types?
*/
String
IDLCodeGenerator::GetNebulaArgType(const String& type) 
{
    if ("string" == type)               return "Scripting::Arg::String";    
    else if ("float4" == type)          return "Scripting::Arg::Float4";
    else if ("matrix44" == type)        return "Scripting::Arg::Matrix44";
    else if ("transform44" == type)     return "Scripting::Arg::Transform44";
    else if ("bool" == type)            return "Scripting::Arg::Bool";
    else if ("guid" == type)            return "Scripting::Arg::Guid";
    else if ("int" == type)             return "Scripting::Arg::Int";
    else if ("uint" == type)            return "Scripting::Arg::UInt";
    else if ("float" == type)           return "Scripting::Arg::Float";
    else if ("object" == type)          return "Scripting::Arg::Object";
    else if ("voidptr" == type)         return "Scripting::Arg::VoidPtr";
    else if ("intArray" == type)        return "Scripting::Arg::IntArray";
    else if ("floatArray" == type)      return "Scripting::Arg::FloatArray";
    else if ("boolArray" == type)       return "Scripting::Arg::BoolArray";
    else if ("stringArray" == type)     return "Scripting::Arg::StringArray";
    else if ("vectorArray" == type)     return "Scripting::Arg::VectorArray";
    else if ("float4Array" == type)     return "Scripting::Arg::Float4Array";
    else if ("matrix44Array" == type)   return "Scripting::Arg::Matrix44Array";
    else if ("guidArray" == type)       return "Scripting::Arg::GuidArray";
    
    else
    {
        this->SetError("IDLCodeGenerator::GetNebulaArgType(): Invalid IDL type '%s'!", type.AsCharPtr());
        return "";
    }
}

//------------------------------------------------------------------------------
/**
    Converts a IDL type string into a Nebula getter method name.
    OBSOLETE??? -> old math types?
*/
String
IDLCodeGenerator::GetNebulaGetterMethod(const String& type) 
{
    if ("string" == type)               return "GetString()";    
    else if ("float4" == type)          return "GetFloat4()";
    else if ("matrix44" == type)        return "GetMatrix44()";
    else if ("transform44" == type)     return "GetTransform44()";
    else if ("bool" == type)            return "GetBool()";
    else if ("guid" == type)            return "GetGuid()";
    else if ("int" == type)             return "GetInt()";
    else if ("uint" == type)            return "GetUInt()";
    else if ("float" == type)           return "GetFloat()";
    else if ("object" == type)          return "GetObject()";
    else if ("voidptr" == type)         return "GetVoidPtr()";
    else if ("intArray" == type)        return "GetIntArray()";
    else if ("floatArray" == type)      return "GetFloatArray()";
    else if ("boolArray" == type)       return "GetBoolArray()";
    else if ("stringArray" == type)     return "GetStringArray()";
    else if ("vectorArray" == type)     return "GetVectorArray()";
    else if ("float4Array" == type)     return "GetFloat4Array()";
    else if ("matrix44Array" == type)   return "GetMatrix44Array()";
    else if ("guidArray" == type)       return "GetGuidArray";
    else
    {
        this->SetError("IDLCodeGenerator::GetNebulaGetterMethod(): Invalid IDL type '%s'!", type.AsCharPtr());
        return "";
    }
}

//------------------------------------------------------------------------------
/**
    Converts a IDL type string into a Nebula setter method name.
    OBSOLETE??? -> old math types?
*/
String
IDLCodeGenerator::GetNebulaSetterMethod(const String& type) 
{
    if ("string" == type)               return "SetString";    
    else if ("float4" == type)          return "SetFloat4";
    else if ("matrix44" == type)        return "SetMatrix44";
    else if ("transform44" == type)     return "SetTransform44";
    else if ("bool" == type)            return "SetBool";
    else if ("guid" == type)            return "SetGuid";
    else if ("int" == type)             return "SetInt";
    else if ("uint" == type)            return "SetUInt";
    else if ("float" == type)           return "SetFloat";
    else if ("object" == type)          return "SetObject";
    else if ("voidptr" == type)         return "SetVoidPtr";
    else if ("intArray" == type)        return "SetIntArray";
    else if ("floatArray" == type)      return "SetFloatArray";
    else if ("boolArray" == type)       return "SetBoolArray";
    else if ("stringArray" == type)     return "SetStringArray";
    else if ("vectorArray" == type)     return "SetVectorArray";
    else if ("float4Array" == type)     return "SetFloat4Array";
    else if ("matrix44Array" == type)   return "SetMatrix44Array";
    else if ("guidArray" == type)       return "SetGuidArray";
    else
    {
        this->SetError("IDLCodeGenerator::GetNebulaSetterMethod(): Invalid IDL type '%s'!", type.AsCharPtr());
        return "";
    }
}

//------------------------------------------------------------------------------
/**
*/
String
IDLCodeGenerator::BuildCallbackPrototype(IDLCommand* cmd, bool withClassName) 
{
    const Util::Array<Ptr<IDLArg>> inArgs = cmd->GetInputArgs();
    const Util::Array<Ptr<IDLArg>> outArgs = cmd->GetOutputArgs();
    n_assert(outArgs.Size() <= 1);
    
    String str;
    if (outArgs.Size() > 0)
    {
        str.Append(this->GetNebulaType(outArgs[0]->GetType()));
    }
    else
    {
        str.Append("void");
    }
    str.Append(" ");
    if (withClassName)
    {
        str.Append(cmd->GetName());
        str.Append("::");
    }
    str.Append("Callback(");
    IndexT inArgIndex;
    for (inArgIndex = 0; inArgIndex < inArgs.Size(); inArgIndex++)
    {
        if (inArgs[inArgIndex]->GetWrappingType().IsEmpty())
        {
            str.Append(this->GetNebulaRefType(inArgs[inArgIndex]->GetType()));
        }
        else
        {
            str.Append(inArgs[inArgIndex]->GetWrappingType());
        }
        str.Append(" ");
        str.Append(inArgs[inArgIndex]->GetName());
        if (inArgIndex < (inArgs.Size() - 1))
        {
            str.Append(", ");
        }
    }
    str.Append(")");
    return str;
}

//------------------------------------------------------------------------------
/**
*/
void
IDLCodeGenerator::WriteIncludeHeader(TextWriter* writer) 
{
    n_assert(0 != writer);
    writer->WriteLine("#pragma once");
    writer->WriteLine("//------------------------------------------------------------------------------");
    writer->WriteLine("/**");
    writer->WriteLine("    This file was generated with Nebula Trifid's idlc compiler tool.");
    writer->WriteLine("    DO NOT EDIT");
    writer->WriteLine("*/");
    if (!this->document->GetLibraries().IsEmpty())
    {
        writer->WriteLine("#include \"scripting/command.h\"");
    }
    if (!this->document->GetProtocols().IsEmpty())
    {
        writer->WriteLine("#include \"messaging/message.h\"");                
    }

    // write dependencies
    const Array<Ptr<IDLLibrary>>& libs = this->document->GetLibraries();
    IndexT libIndex;
    for (libIndex = 0; libIndex < libs.Size(); libIndex++)
    {
        const Ptr<IDLLibrary>& curLib = libs[libIndex];
        IndexT depIndex;
        for (depIndex = 0; depIndex < curLib->GetDependencies().Size(); depIndex++)
        {
            IDLDependency* dep = curLib->GetDependencies()[depIndex];
            writer->WriteFormatted("#include \"%s\"\n", dep->GetHeader().AsCharPtr());
        }
    }
    const Array<Ptr<IDLProtocol>>& protocols = this->document->GetProtocols();
    IndexT protIndex;
    for (protIndex = 0; protIndex < protocols.Size(); protIndex++)
    {
        const Ptr<IDLProtocol>& curProt = protocols[protIndex];
        IndexT depIndex;
        for (depIndex = 0; depIndex < curProt->GetDependencies().Size(); depIndex++)
        {
            IDLDependency* dep = curProt->GetDependencies()[depIndex];
            writer->WriteFormatted("#include \"%s\"\n", dep->GetHeader().AsCharPtr());
        }
    }
    /// Write dependencies from the AttributeLib's too! ^^
    const Array<Ptr<IDLAttributeLib>>& attributes = this->document->GetAttributeLibs();
    IndexT attrIndex;
    for (attrIndex = 0; attrIndex < attributes.Size(); attrIndex++)
    {
        const Ptr<IDLAttributeLib>& curAttrLib = attributes[attrIndex];
        IndexT depIndex;
        for (depIndex = 0; depIndex < curAttrLib->GetDependencies().Size(); depIndex++)
        {
            IDLDependency* dep = curAttrLib->GetDependencies()[depIndex];
            writer->WriteFormatted("#include \"%s\"\n", dep->GetHeader().AsCharPtr());
        }
    }
    writer->WriteLine("");
}

//------------------------------------------------------------------------------
/**
*/
void
IDLCodeGenerator::WriteLibraryDeclarations(TextWriter* writer) 
{
    n_assert(0 != writer);
    if (!this->document->GetLibraries().IsEmpty())
    {
        writer->WriteLine("//------------------------------------------------------------------------------");
        writer->WriteLine("namespace Commands");
        writer->WriteLine("{");

        // for each library definition...
        const Array<Ptr<IDLLibrary>>& libs = this->document->GetLibraries();
        IndexT libIndex;
        SizeT numLibs = libs.Size();
        for (libIndex = 0; libIndex < numLibs; libIndex++)
        {
            IDLLibrary* curLib = libs[libIndex];

            // write library class definition
            writer->WriteFormatted("class %s\n", curLib->GetName().AsCharPtr());
            writer->WriteLine("{");
            writer->WriteLine("public:");
            writer->WriteLine("    /// register commands");
            writer->WriteLine("    static void Register();");
            writer->WriteLine("};");

            // for each command in the library...
            const Array<Ptr<IDLCommand>>& cmds = curLib->GetCommands();
            IndexT cmdIndex;
            SizeT numCmds = cmds.Size();
            for (cmdIndex = 0; cmdIndex < numCmds; cmdIndex++)
            {
                this->WriteCommandDeclaration(cmds[cmdIndex], writer);
            }
        }
        writer->WriteLine("} // namespace Commands");
    }
}

//------------------------------------------------------------------------------
/**
    FIXME: hmm, enable different namespaces for message protocols?
*/
bool 
IDLCodeGenerator::WriteProtocolDeclarations(IO::TextWriter* writer) 
{
    n_assert(0 != writer);
    if (!this->document->GetProtocols().IsEmpty())
    {
        // for each protocol definition...
        const Array<Ptr<IDLProtocol>>& protocols = this->document->GetProtocols();
        IndexT protocolIndex;
        SizeT numProtocols = protocols.Size();
        for (protocolIndex = 0; protocolIndex < numProtocols; protocolIndex++)
        {
            IDLProtocol* curProtocol = protocols[protocolIndex];

            writer->WriteLine("//------------------------------------------------------------------------------");
            writer->WriteFormatted("namespace %s\n", curProtocol->GetNameSpace().AsCharPtr());
            writer->WriteLine("{");

            // for each message in the protocol:
            const Array<Ptr<IDLMessage>>& msgs = curProtocol->GetMessages();
            IndexT msgIndex;
            for (msgIndex = 0; msgIndex < msgs.Size(); msgIndex++)
            {
                if (!this->WriteMessageDeclaration(curProtocol, msgs[msgIndex], writer))
                    return false;
            }
            writer->WriteFormatted("} // namespace %s\n", curProtocol->GetNameSpace().AsCharPtr());
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
IDLCodeGenerator::WriteCommandDeclaration(IDLCommand* cmd, TextWriter* writer) 
{
    n_assert(0 != cmd);
    n_assert(0 != writer);

    writer->WriteLine("//------------------------------------------------------------------------------");
    writer->WriteFormatted("class %s : public Scripting::Command\n", cmd->GetName().AsCharPtr());
    writer->WriteLine("{");
    writer->WriteFormatted("    __DeclareClass(%s);\n", cmd->GetName().AsCharPtr());
    writer->WriteLine("public:");
    writer->WriteLine("    virtual void OnRegister();");
    writer->WriteLine("    virtual bool OnExecute();");
    writer->WriteLine("    virtual Util::String GetHelp() const;");
    writer->WriteLine("private:");
    writer->WriteLine("    " + this->BuildCallbackPrototype(cmd, false) + ";");
    writer->WriteLine("};");
    writer->WriteLine("");
}

//------------------------------------------------------------------------------
/**
*/
bool 
IDLCodeGenerator::WriteMessageDeclaration(IDLProtocol* prot, IDLMessage* msg, IO::TextWriter* writer) 
{
    n_assert(0 != prot);
    n_assert(0 != msg);
    n_assert(0 != writer);

    // build parent class name
    String parentClass = msg->GetParentClass();
    n_assert(parentClass.IsValid());

    writer->WriteLine("//------------------------------------------------------------------------------");
    writer->WriteFormatted("class %s : public %s\n", msg->GetName().AsCharPtr(), parentClass.AsCharPtr());
    writer->WriteLine("{");
    writer->WriteFormatted("    __DeclareClass(%s);\n", msg->GetName().AsCharPtr());
    writer->WriteLine("    __DeclareMsgId;");    
    writer->WriteLine("public:");
    
    // write constructor 
    String initList;
    Array<Ptr<IDLArg>> args = msg->GetInputArgs();
    args.AppendArray(msg->GetOutputArgs());
    IndexT i;
    for (i = 0; i < args.Size(); i++)
    {
        if (args[i]->GetDefaultValue().IsValid())
        {
            if (!initList.IsEmpty())
            {
                initList.Append(",\n");
            }
            String argMemberName = args[i]->GetName();
            argMemberName.ToLower();
            initList.Append("        ");
            initList.Append(argMemberName);
            initList.Append("(");
            initList.Append(args[i]->GetDefaultValue());
            initList.Append(")");
        }
    }
    writer->WriteFormatted("    %s() %s\n", msg->GetName().AsCharPtr(), initList.IsValid() ? ":" : "");
    if (initList.IsValid())
    {
        writer->WriteString(initList);
        writer->WriteLine("");
    }
    writer->WriteLine("    { };");

    // write input args
    const Array<Ptr<IDLArg>>& inArgs = msg->GetInputArgs();
    IndexT argIndex;
    for (argIndex = 0; argIndex < inArgs.Size(); argIndex++)
    {
        this->WriteMessageArg(prot, msg, inArgs[argIndex], writer, true);
    }

    // write output args
    const Array<Ptr<IDLArg>>& outArgs = msg->GetOutputArgs();
    for (argIndex = 0; argIndex < outArgs.Size(); argIndex++)
    {
        this->WriteMessageArg(prot, msg, outArgs[argIndex], writer, false);
    }

    // write encode and decode 
    if (!this->WriteEncodeImplementation(msg, writer) || !this->WriteDecodeImplementation(msg, writer))
    {
        return false;
    }

    writer->WriteLine("};");
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
IDLCodeGenerator::WriteMessageArg(IDLProtocol* prot, IDLMessage* msg, IDLArg* arg, TextWriter* writer, bool isInputArg) 
{
    n_assert(0 != arg);
    n_assert(0 != writer);

    // build some string which are needed multiple times
    String argMemberName = arg->GetName();
    argMemberName.ToLower();
    String argTypeString;
    if ((arg->GetType().FindStringIndex("::") != InvalidIndex) &&
        (arg->GetType().FindStringIndex("*") == InvalidIndex))
    {
        // a complex type, pass by reference
        argTypeString.Append("const ");
        argTypeString.Append(arg->GetType());
        argTypeString.Append("&");
    }
    else
    {
        // a simple builtin type
        argTypeString = arg->GetType();
    }

    // write setter
    String str;
    str.Append("public:\n");
    str.Append("    void Set");
    str.Append(arg->GetName());
    str.Append("(");
    str.Append(argTypeString);
    str.Append(" val)\n");
    str.Append("    {\n");
    str.Append("        n_assert(!this->handled);\n");    
    str.Append("        this->");
    str.Append(argMemberName);
    str.Append(" = val;\n");
    str.Append("    };\n");

    // write getter
    str.Append("    ");
    str.Append(argTypeString);
    str.Append(" Get");
    str.Append(arg->GetName());
    str.Append("() const\n");
    str.Append("    {\n");
    if ((!isInputArg))
    {
        str.Append("        n_assert(this->handled);\n");
    }
    str.Append("        return this->");
    str.Append(argMemberName);
    str.Append(";\n");
    str.Append("    };\n");

    // write member
    str.Append("private:\n");
    str.Append("    ");
    str.Append(arg->GetType());
    str.Append(" ");
    str.Append(argMemberName);
    str.Append(";\n");
    
    // write setter, getter and member to TextWriter
    writer->WriteString(str);
}


//------------------------------------------------------------------------------
/**
    
*/
void
IDLCodeGenerator::WriteAttributeLibraryDeclaration(TextWriter* writer) 
{
    if (!this->document->GetAttributeLibs().IsEmpty())
    {

        writer->WriteLine("#include \"attr/attrid.h\"");
        writer->WriteLine("#include \"attr/attributedefinition.h\"");

        const Array<Ptr<IDLAttributeLib>>& attributeLibs = this->document->GetAttributeLibs();
        IndexT attrlIndex;
        for (attrlIndex = 0; attrlIndex < attributeLibs.Size(); attrlIndex++)
        {
            const Ptr<IDLAttributeLib>& curAttrl = attributeLibs[attrlIndex];

            writer->WriteLine("namespace Attr");
            writer->WriteLine("{");
            IndexT aIndex;
            for (aIndex = 0; aIndex < curAttrl->GetAttributes().Size(); aIndex++)
            {
                const Ptr<IDLAttribute>& attr = curAttrl->GetAttributes()[aIndex];

                writer->WriteFormatted("    Declare%s(%s, '%s', %s);\n",
                    attr->GetType().AsCharPtr(),
                    attr->GetName().AsCharPtr(),
                    attr->GetFourCC().AsCharPtr(),
                    attr->GetAccessMode().AsCharPtr());

            }
            writer->WriteLine("} // attr");
        }
    }
}
//------------------------------------------------------------------------------
/**
*/
void
IDLCodeGenerator::WriteIncludeFooter(TextWriter* writer) 
{
    n_assert(0 != writer);
    writer->WriteLine("//------------------------------------------------------------------------------");
}

//------------------------------------------------------------------------------
/**
*/
void
IDLCodeGenerator::WriteSourceHeader(TextWriter* writer) 
{
    n_assert(0 != writer);
    writer->WriteLine("//------------------------------------------------------------------------------");
    writer->WriteLine("//  MACHINE GENERATED, DON'T EDIT!");
    writer->WriteLine("//------------------------------------------------------------------------------");
    
    URI headerUri = this->BuildHeaderUri();
    String headerFile = headerUri.AsString().ExtractFileName();
    writer->WriteLine("#include \"stdneb.h\"");
    if (!this->document->GetLibraries().IsEmpty())
    {
        writer->WriteLine("#include \"scripting/scriptserver.h\"");
        writer->WriteLine("#include \"scripting/arg.h\"");
    }

    /// Include this header
    writer->WriteFormatted("#include \"%s\"\n", headerFile.AsCharPtr());
    writer->WriteLine("");

    if (!this->document->GetAttributeLibs().IsEmpty())
    {
        writer->WriteLine("#include \"attr/attribute.h\"");
        /// Write the additional attributeLib dependencies here too, since they will be needed when registering the inherited attributes
        const Array<Ptr<IDLAttributeLib>>& attributeLibs = this->document->GetAttributeLibs();
        IndexT attrlIndex;
        for (attrlIndex = 0; attrlIndex < attributeLibs.Size(); attrlIndex++)
        {
            const Ptr<IDLAttributeLib>& curAttrl = attributeLibs[attrlIndex];
            writer->WriteLine("// Additional includes by AttributeLib");
            Util::Array<Ptr<Tools::IDLDependency>> dependencies = curAttrl->GetDependencies();
            IndexT includeIndex;
            for (includeIndex = 0; includeIndex < dependencies.Size(); ++includeIndex) {
                Ptr<Tools::IDLDependency> depencency = dependencies[includeIndex];
                writer->WriteFormatted("#include \"%s\"\n", depencency->GetHeader().AsCharPtr());
            }
        }
    }
    writer->WriteLine("");

    // write __ImplementClass macros
    if (!this->document->GetLibraries().IsEmpty())
    {
        const Array<Ptr<IDLLibrary>>& libs = this->document->GetLibraries();
        IndexT libIndex;
        for (libIndex = 0; libIndex < libs.Size(); libIndex++)
        {
            const Ptr<IDLLibrary>& curLib = libs[libIndex];
            IndexT cmdIndex;
            for (cmdIndex = 0; cmdIndex < curLib->GetCommands().Size(); cmdIndex++)
            {
                const Ptr<IDLCommand>& cmd = curLib->GetCommands()[cmdIndex];
                writer->WriteFormatted("__ImplementClass(Commands::%s, '%s', Scripting::Command);\n",
                    cmd->GetName().AsCharPtr(), cmd->GetFourCC().AsCharPtr());
            }
        }
    }
    if (!this->document->GetProtocols().IsEmpty())
    {
        const Array<Ptr<IDLProtocol>>& protocols = this->document->GetProtocols();
        IndexT protIndex;
        for (protIndex = 0; protIndex < protocols.Size(); protIndex++)
        {
            const Ptr<IDLProtocol>& curProt = protocols[protIndex];
            writer->WriteFormatted("namespace %s\n", curProt->GetNameSpace().AsCharPtr());
            writer->WriteLine("{");
            IndexT msgIndex;
            for (msgIndex = 0; msgIndex < curProt->GetMessages().Size(); msgIndex++)
            {
                const Ptr<IDLMessage>& msg = curProt->GetMessages()[msgIndex];
                writer->WriteFormatted("    __ImplementClass(%s::%s, '%s', %s);\n",
                    curProt->GetNameSpace().AsCharPtr(),
                    msg->GetName().AsCharPtr(),
                    msg->GetFourCC().AsCharPtr(),
                    msg->GetParentClass().AsCharPtr());
                writer->WriteFormatted("    __ImplementMsgId(%s);\n", msg->GetName().AsCharPtr());
            }
            writer->WriteFormatted("} // %s\n", curProt->GetNameSpace().AsCharPtr());
        }
    }
    if (!this->document->GetAttributeLibs().IsEmpty())
    {
        writer->WriteLine("// Defining AttributeLib");
        const Array<Ptr<IDLAttributeLib>>& attributeLibs = this->document->GetAttributeLibs();
        IndexT attrlIndex;
        for (attrlIndex = 0; attrlIndex < attributeLibs.Size(); attrlIndex++)
        {
            const Ptr<IDLAttributeLib>& curAttrl = attributeLibs[attrlIndex];

            writer->WriteLine("namespace Attr");
            writer->WriteLine("{");
            IndexT aIndex;
            for (aIndex = 0; aIndex < curAttrl->GetAttributes().Size(); aIndex++)
            {
                const Ptr<IDLAttribute>& attr = curAttrl->GetAttributes()[aIndex];
                if (attr->HasDefault())
                {
                    writer->WriteFormatted("    Define%sWithDefault(%s, '%s', %s, %s);\n",
                        attr->GetType().AsCharPtr(),
                        attr->GetName().AsCharPtr(),
                        attr->GetFourCC().AsCharPtr(),
                        attr->GetAccessMode().AsCharPtr(),
                        attr->GetDefault().AsCharPtr());
                }
                else
                {
                    writer->WriteFormatted("    Define%s(%s, '%s', %s);\n",
                        attr->GetType().AsCharPtr(),
                        attr->GetName().AsCharPtr(),
                        attr->GetFourCC().AsCharPtr(),
                        attr->GetAccessMode().AsCharPtr());
                }

            }
            writer->WriteLine("} // attr");
        }
    }
    if (!this->document->GetProperties().IsEmpty())
    {
        writer->WriteLine("#include \"basegamefeature/managers/categorymanager.h\"");
        const Array<Ptr<IDLProperty>>& props = this->document->GetProperties();
        IndexT propIndex;
        for (propIndex = 0; propIndex < props.Size(); propIndex++)
        {
            const Ptr<IDLProperty>& curProp = props[propIndex];

            if (!curProp->GetHeader().IsEmpty())
            {
                writer->WriteFormatted("#include \"%s\"\n", curProp->GetHeader().AsCharPtr());
            }

            writer->WriteFormatted("void %s::SetupExternalAttributes()\n{\n", curProp->GetName().AsCharPtr());
            const Array<Util::String> & propAttrs = curProp->GetAttributes();
            const Array<bool> & serialize = curProp->GetSerialize();
            IndexT paIndex;
            for (paIndex = 0; paIndex < propAttrs.Size(); paIndex++)
            {
                Util::String serializeString = serialize[paIndex] ? "true" : "false";
                writer->WriteFormatted("	SetupAttr(Attr::%s, %s);\n", propAttrs[paIndex].AsCharPtr(), serializeString.AsCharPtr());
            }
            if (!curProp->GetParentClass().IsEmpty())
            {
                writer->WriteFormatted("	%s::SetupExternalAttributes();\n", curProp->GetParentClass().AsCharPtr());
            }
            writer->WriteLine("}\n");
        }
    }
    writer->WriteLine("");
}

//------------------------------------------------------------------------------
/**
*/
void
IDLCodeGenerator::WriteSourceFooter(TextWriter* writer) 
{
    n_assert(0 != writer);
    writer->WriteLine("//------------------------------------------------------------------------------");
}

//------------------------------------------------------------------------------
/**
*/
void
IDLCodeGenerator::WriteLibraryImplementations(TextWriter* writer) 
{
    n_assert(0 != writer);
    
    writer->WriteLine("namespace Commands");
    writer->WriteLine("{");

    // for each library definition...
    const Array<Ptr<IDLLibrary>>& libs = this->document->GetLibraries();
    IndexT libIndex;
    SizeT numLibs = libs.Size();
    for (libIndex = 0; libIndex < numLibs; libIndex++)
    {
        IDLLibrary* curLib = libs[libIndex];
        writer->WriteLine("//------------------------------------------------------------------------------");
        writer->WriteLine("/**");
        writer->WriteLine("*/");
        writer->WriteLine("void");
        writer->WriteFormatted("%s::Register()\n", curLib->GetName().AsCharPtr());
        writer->WriteLine("{");
        writer->WriteLine("    Scripting::ScriptServer* scriptServer = Scripting::ScriptServer::Instance();");
        IndexT cmdIndex;
        for (cmdIndex = 0; cmdIndex < curLib->GetCommands().Size(); cmdIndex++)
        {
            IDLCommand* cmd = curLib->GetCommands()[cmdIndex];
            String scriptCommandName = cmd->GetName();
            scriptCommandName.ToLower();
            writer->WriteFormatted("    scriptServer->RegisterCommand(\"%s\", %s::Create());\n", 
                scriptCommandName.AsCharPtr(), cmd->GetName().AsCharPtr());
        }
        writer->WriteLine("}");
        writer->WriteLine("");

        // write command implementations
        for (cmdIndex = 0; cmdIndex < curLib->GetCommands().Size(); cmdIndex++)
        {
            IDLCommand* cmd = curLib->GetCommands()[cmdIndex];
            this->WriteCommandImplementation(cmd, writer);
        }
    }
    writer->WriteLine("} // namespace Commands");
}

//------------------------------------------------------------------------------
/**
*/
void
IDLCodeGenerator::WriteCommandImplementation(IDLCommand* cmd, TextWriter* writer) 
{
    n_assert(0 != cmd);
    n_assert(0 != writer);

    // OnRegister()
    writer->WriteLine("//------------------------------------------------------------------------------");
    writer->WriteLine("/**");
    writer->WriteLine("*/");
    writer->WriteLine("void");
    writer->WriteFormatted("%s::OnRegister()\n", cmd->GetName().AsCharPtr());
    writer->WriteLine("{");
    writer->WriteLine("    Scripting::Command::OnRegister();");
    IndexT inArgIndex;
    for (inArgIndex = 0; inArgIndex < cmd->GetInputArgs().Size(); inArgIndex++)
    {
        IDLArg* arg = cmd->GetInputArgs()[inArgIndex];
        const Util::String & argType = arg->GetWrappingType().IsEmpty()?arg->GetType():arg->GetWrappingType();
        writer->WriteFormatted("    this->args.AddArg(\"%s\", %s);\n", 
            arg->GetName().AsCharPtr(), this->GetNebulaArgType(argType).AsCharPtr());
    }
    IndexT outArgIndex;
    for (outArgIndex = 0; outArgIndex < cmd->GetOutputArgs().Size(); outArgIndex++)
    {
        IDLArg* arg = cmd->GetOutputArgs()[outArgIndex];
        const Util::String & argType = arg->GetWrappingType().IsEmpty()?arg->GetType():arg->GetWrappingType();
        writer->WriteFormatted("    this->results.AddArg(\"%s\", %s);\n",
            arg->GetName().AsCharPtr(), this->GetNebulaArgType(argType).AsCharPtr());
    }
    writer->WriteLine("}");
    writer->WriteLine("");

    // GetHelp()
    writer->WriteLine("//------------------------------------------------------------------------------");
    writer->WriteLine("/**");
    writer->WriteLine("*/");
    writer->WriteLine("Util::String");
    writer->WriteFormatted("%s::GetHelp() const\n", cmd->GetName().AsCharPtr());
    writer->WriteLine("{");
    writer->WriteFormatted("    return \"%s\";\n", cmd->GetDesc().AsCharPtr());
    writer->WriteLine("}");
    writer->WriteLine("");

    // OnExecute()
    writer->WriteLine("//------------------------------------------------------------------------------");
    writer->WriteLine("/**");
    writer->WriteLine("*/");
    writer->WriteLine("bool");
    writer->WriteFormatted("%s::OnExecute()\n", cmd->GetName().AsCharPtr());
    writer->WriteLine("{");
    
    // marshal input args
    for (inArgIndex = 0; inArgIndex < cmd->GetInputArgs().Size(); inArgIndex++)
    {
        IDLArg* arg = cmd->GetInputArgs()[inArgIndex];
        const Util::String & argType = arg->GetWrappingType().IsEmpty()?arg->GetType():arg->GetWrappingType();
        writer->WriteFormatted("    %s %s = this->args.GetArgValue(%d).%s;\n",
            this->GetNebulaRefType(argType).AsCharPtr(),
            arg->GetName().AsCharPtr(),
            inArgIndex,
            this->GetNebulaGetterMethod(argType).AsCharPtr());
    }

    // generate callback method call
    String str;
    str.Append("    ");
    if (cmd->GetOutputArgs().Size() > 0)
    {
        str.Append(this->GetNebulaType(cmd->GetOutputArgs()[0]->GetType()));
        str.Append(" ");
        str.Append(cmd->GetOutputArgs()[0]->GetName());
        str.Append(" = ");
    }
    str.Append("this->Callback(");
    for (inArgIndex = 0; inArgIndex < cmd->GetInputArgs().Size(); inArgIndex++)
    {
        IDLArg* arg = cmd->GetInputArgs()[inArgIndex];
        str.Append(arg->GetName().AsCharPtr());
        if (inArgIndex < (cmd->GetInputArgs().Size() - 1))
        {
            str.Append(", ");
        }
    }
    str.Append(");");
    writer->WriteLine(str);

    // optionally set result
    if (cmd->GetOutputArgs().Size() > 0)
    {
        IDLArg* arg = cmd->GetOutputArgs()[0];
        writer->WriteFormatted("    this->results.ArgValue(0).%s(%s);\n",
            this->GetNebulaSetterMethod(arg->GetType()).AsCharPtr(),
            arg->GetName().AsCharPtr());
    }
    writer->WriteLine("    return true;");
    writer->WriteLine("}");
    writer->WriteLine("");

    // Callback() method
    writer->WriteLine("//------------------------------------------------------------------------------");
    writer->WriteLine("/**");
    writer->WriteLine("*/");
    writer->WriteLine(this->BuildCallbackPrototype(cmd, true));
    writer->WriteLine("{");
    writer->WriteLine(cmd->GetCode());
    writer->WriteLine("}");
    writer->WriteLine("");
}

//------------------------------------------------------------------------------
/**
*/
bool
IDLCodeGenerator::WriteEncodeImplementation(IDLMessage* msg, IO::TextWriter* writer)
{
    String str;
    str.Append("public:\n");
    str.Append("    void Encode");    
    str.Append("(const Ptr<IO::BinaryWriter>& writer)\n");
    str.Append("    {\n");    
    
    bool writeString = false;
    // check all input args
    const Array<Ptr<IDLArg>>& inArgs = msg->GetInputArgs();
    IndexT argIndex;
    for (argIndex = 0; argIndex < inArgs.Size(); argIndex++)
    {
        if (inArgs[argIndex]->IsSerialized())
        {
            String type = inArgs[argIndex]->GetType();           
            String name = inArgs[argIndex]->GetName();
            if (!this->TypeEncode(type, name, inArgs[argIndex], str))
            {
                return false;
            }
            writeString = true;                
        } 
    }    
    // call subclass encode function
    if (writeString && msg->GetParentClass().IsValid())
    {
        str.Append("        " + msg->GetParentClass() + "::Encode(writer);\n");
    }
    str.Append("    };\n");   

    if (writeString)
    {
        writer->WriteString(str);
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool 
IDLCodeGenerator::WriteDecodeImplementation(IDLMessage* msg, IO::TextWriter* writer) 
{
    String str;
    str.Append("public:\n");
    str.Append("    void Decode");    
    str.Append("(const Ptr<IO::BinaryReader>& reader)\n");
    str.Append("    {\n");    

    bool writeString = false;
    // check all input args
    const Array<Ptr<IDLArg>>& inArgs = msg->GetInputArgs();
    IndexT argIndex;
    for (argIndex = 0; argIndex < inArgs.Size(); argIndex++)
    {
        if (inArgs[argIndex]->IsSerialized())
        {
            String type = inArgs[argIndex]->GetType();                        
            String name = inArgs[argIndex]->GetName();
            if (!this->TypeDecode(type, name, inArgs[argIndex], str))
            {
                return false;
            }
            writeString = true; 
        }
    }     
    // call subclass encode function
    if (writeString && msg->GetParentClass().IsValid())
    {
        str.Append("        " + msg->GetParentClass() + "::Decode(reader);\n");
    }

    str.Append("    };\n");   

    if (writeString)
    {
        writer->WriteString(str);
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
Util::String 
IDLCodeGenerator::ConvertToCamelNotation(const Util::String& lowerCaseType) 
{
    if (lowerCaseType == "char") return "Char";
    else if (lowerCaseType == "float") return "Float";                                 
    else if (lowerCaseType == "int") return "Int";
    else if (lowerCaseType == "uint") return "UInt";
    else if (lowerCaseType == "bool") return "Bool";   
    else if (lowerCaseType == "double") return "Double";
    else if (lowerCaseType == "short") return "Short";
    else if (lowerCaseType == "ushort") return "UShort";    
    else if (lowerCaseType == "string" || lowerCaseType == "String" || lowerCaseType == "Util::String") return "String";  // class names are uppercase 
    else if (lowerCaseType == "matrix44" || lowerCaseType == "Matrix44" || lowerCaseType == "Math::matrix44") return "Matrix44";
    else if (lowerCaseType == "float2" || lowerCaseType == "Float2" || lowerCaseType == "Math::float2") return "Float2";
    else if (lowerCaseType == "float4" || lowerCaseType == "Float4" || lowerCaseType == "Math::float4") return "Float4";
    else if (lowerCaseType == "point" || lowerCaseType == "Point" || lowerCaseType == "Math::point") return "Point";
    else if (lowerCaseType == "vector" || lowerCaseType == "Vector" || lowerCaseType == "Math::vector") return "Vector";
    else if (lowerCaseType == "blob" || lowerCaseType == "Blob" || lowerCaseType == "Util::Blob") return "Blob";
    else if (lowerCaseType == "guid" || lowerCaseType == "Guid" || lowerCaseType == "Util::Guid") return "Guid";
    else if (lowerCaseType == "transform44" || lowerCaseType == "Transform44" || lowerCaseType == "Math::transform44") return "Transform44";
    else if (lowerCaseType == "IndexT" || lowerCaseType == "Tick" || lowerCaseType == "Timing::Tick") return "Int";
    else if (lowerCaseType == "int64_t") return "Int64";
    else if (lowerCaseType == "uint64_t") return "UInt64";

   // n_error("Invalid type %s", lowerCaseType.AsCharPtr());
    return "";
}

//------------------------------------------------------------------------------
/**
*/
bool 
IDLCodeGenerator::TypeEncode(const Util::String & type, const Util::String & name, const Ptr<IDLArg> & arg, Util::String & target)
{
    Util::String newtype = this->ConvertToCamelNotation(type);
    if (newtype.Length())
    {
        // check valid type
        if (!IDLArg::IsValidType(newtype))
        {
            this->SetError("IDLCodeGenerator::TypeEncode: type %s not valid for serialization!!!", newtype.AsCharPtr());
            return false;
        }
        target.Append("        writer->Write" + newtype + "(this->Get" + name + "());\n");
    }
    else
    {
        if (arg->GetWrappingType().Length())
        {
            Util::String wrap = this->ConvertToCamelNotation(arg->GetWrappingType());
            target.Append("        writer->Write" + wrap + "((" + arg->GetWrappingType() + ")this->Get" + name + "());\n");
        }
        else if (type == "StringAtom" || type == "Util::StringAtom")
        {
            target.Append("        writer->WriteString(this->Get" + name + "().Value());\n");
        }
        else if (type == "Ptr<Game::Entity>")
        {
            target.Append("        writer->WriteUInt64(__GetNetworkID(this->Get" + name + "()));\n");
        }
        else
        {            
            this->SetError("IDLCodeGenerator::TypeEncode: type %s not valid for serialization!!!", type.AsCharPtr());
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
IDLCodeGenerator::TypeDecode(const Util::String & type, const Util::String & name, const Ptr<IDLArg> & arg, Util::String & target)
{
    Util::String newtype = this->ConvertToCamelNotation(type);
    if (newtype.Length())
    {
        // check valid type
        if (!IDLArg::IsValidType(newtype))
        {
            n_printf("foo\n");
            this->SetError("IDLCodeGenerator::TypeDecode: type %s not valid for serialization!!!", type.AsCharPtr());
            return false;
        }
        target.Append("        this->Set" + name + "(reader->Read" + newtype + "());\n");
    }
    else
    {
        if (arg->GetWrappingType().Length())
        {
            target.Append("        this->Set" + name + "((" + arg->GetType() + ")reader->Read" + this->ConvertToCamelNotation(arg->GetWrappingType()) + "());\n");
        }
        else if (type == "StringAtom" || type == "Util::StringAtom")
        {
            target.Append("        this->Set" + name + "(reader->ReadString());\n");
        }
        else if (type == "Ptr<Game::Entity>")
        {
            target.Append("        this->Set" + name + "(MultiplayerFeature::NetworkServer::Instance()->GetEntityByNetworkID(reader->ReadUInt64()));\n");
        }
        else
        {
            n_printf("foo2\n");
            this->SetError("IDLCodeGenerator::TypeDecode: type %s not valid for serialization!!!", type.AsCharPtr());
            return false;
        }
    }
    return true;
}
} // namespace Tools
