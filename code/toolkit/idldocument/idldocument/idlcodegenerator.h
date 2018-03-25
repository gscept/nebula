#pragma once
#ifndef TOOLS_IDLCODEGENERATOR_H
#define TOOLS_IDLCODEGENERATOR_H
//------------------------------------------------------------------------------
/**
    @class Tools::IDLCodeGenerator
    
    Generate C++ source code from the parsed IDL object structure.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "io/uri.h"
#include "io/textwriter.h"
#include "idldocument/idldocument.h"

//------------------------------------------------------------------------------
namespace Tools
{
class IDLCodeGenerator : public Core::RefCounted
{
    __DeclareClass(IDLCodeGenerator);
public:
    /// constructor
    IDLCodeGenerator();
    /// set the output base filename
    void SetURI(const IO::URI& uri);
    /// get the output base filename
    const IO::URI& GetURI() const;
    /// build the header file uri
    IO::URI BuildHeaderUri() const;
    /// build the source file uri
    IO::URI BuildSourceUri() const;
    /// set the parser object tree
    void SetDocument(IDLDocument* doc);
    /// get the parser object tree
    IDLDocument* GetDocument() const;
    /// generate the include file
    bool GenerateIncludeFile();
    /// generate the source file
    bool GenerateSourceFile();
    /// check for error
    bool HasError() const;
    /// get error message
    const Util::String& GetError() const;
    /// set an optional output folder
    void SetOutputURI(const IO::URI& uri);

private:
    /// set an error string
    void __cdecl SetError(const char* fmt, ...);
    /// convert IDL data type to Nebula C++ reference type
    Util::String GetNebulaRefType(const Util::String& idlType) ;
    /// convert IDL data type to Nebula C++ type
    Util::String GetNebulaType(const Util::String& idlType) ;
    /// convert IDL data type to Nebula Arg::Type
    Util::String GetNebulaArgType(const Util::String& idlType) ;
    /// convert IDL data type to Nebula getter method
    Util::String GetNebulaGetterMethod(const Util::String& idlType) ;
    /// convert IDL data type to Nebula setter method
    Util::String GetNebulaSetterMethod(const Util::String& idlType) ;
    /// build a call back C++ function prototype
    Util::String BuildCallbackPrototype(IDLCommand* cmd, bool withClassName) ;
    /// write header for include file
    void WriteIncludeHeader(IO::TextWriter* writer) ;
    /// write command library declaration to include file
    void WriteLibraryDeclarations(IO::TextWriter* writer) ;
    /// write message protocol declaration to include file
    bool WriteProtocolDeclarations(IO::TextWriter* writer) ;
    /// write a command declaration to include file
    void WriteCommandDeclaration(IDLCommand* cmd, IO::TextWriter* writer) ;
    /// write a message declaration to include file
    bool WriteMessageDeclaration(IDLProtocol* prot, IDLMessage* msg, IO::TextWriter* writer) ;
    /// write declaration statements for a single message to the include file
    void WriteMessageArg(IDLProtocol* prot, IDLMessage* msg, IDLArg* arg, IO::TextWriter* writer, bool isInputArg) ;
    /// write footer for include file
    void WriteIncludeFooter(IO::TextWriter* writer) ;
    /// write the source header
    void WriteSourceHeader(IO::TextWriter* writer) ;
    /// write library implementation to source file
    void WriteLibraryImplementations(IO::TextWriter* writer) ;
    /// write source file footer to source file
    void WriteSourceFooter(IO::TextWriter* writer) ;
    /// write a command implementation to the source file
    void WriteCommandImplementation(IDLCommand* cmd, IO::TextWriter* writer) ;
    /// write encode function
    bool WriteEncodeImplementation(IDLMessage* msg, IO::TextWriter* writer);
    /// write decode function
    bool WriteDecodeImplementation(IDLMessage* msg, IO::TextWriter* writer);
    /// convert type to streamwriter type
    Util::String ConvertToCamelNotation(const Util::String& lowerCaseType) ;
    /// write attribute declarations to include file
    void WriteAttributeLibraryDeclaration(IO::TextWriter* writer) ;
    /// write encode function for a given type
    bool TypeEncode(const Util::String & type, const Util::String & name, const Ptr<IDLArg> & arg, Util::String & target);
    /// write decode function for a given type
    bool TypeDecode(const Util::String & type, const Util::String & name, const Ptr<IDLArg> & arg, Util::String & target);

    IO::URI uri;
    IO::URI outputUri;
    Util::String error;
    bool hasError;
    Ptr<IDLDocument> document;
};

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLCodeGenerator::GetError() const
{
    return this->error;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
IDLCodeGenerator::HasError() const
{
    return this->hasError;
}

//------------------------------------------------------------------------------
/**
*/
inline void
IDLCodeGenerator::SetURI(const IO::URI& u)
{
    this->uri = u;
    n_assert(this->uri.IsValid());
}

//------------------------------------------------------------------------------
/**
*/
inline const IO::URI&
IDLCodeGenerator::GetURI() const
{
    return this->uri;
}

//------------------------------------------------------------------------------
/**
*/
inline void
IDLCodeGenerator::SetDocument(IDLDocument* doc)
{
    n_assert(0 != doc);
    this->document = doc;
    this->error = "";
    this->hasError = false;
}

//------------------------------------------------------------------------------
/**
*/
inline IDLDocument*
IDLCodeGenerator::GetDocument() const
{
    return this->document;
}

//------------------------------------------------------------------------------
/**
*/
inline void
IDLCodeGenerator::SetOutputURI(const IO::URI& u)
{
	this->outputUri = u;
	n_assert(this->outputUri.IsValid());
}
} // namespace Tools
//------------------------------------------------------------------------------
#endif
