#pragma once
#ifndef TOOLS_IDLCOMPILER_H
#define TOOLS_IDLCOMPILER_H
//------------------------------------------------------------------------------
/**
    @class Tools::IDLCompiler
  
    Compile Nebula3's IDL (Messaging Markup Language) into C++ source files.
    IDL files are XML files which must conform to the interface.xsd schema. The 
    IDLCompiler doesn't check too hard whether the file conforms to the
    IDL schema, so it's best to use an XML editor which performs the 
    checks, like Altova XMLSpy.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "app/consoleapplication.h"
#include "idldocument/idldocument.h"
#include "io/uri.h"

//------------------------------------------------------------------------------
namespace Tools
{
class IDLCompiler : public App::ConsoleApplication
{
public:
    /// constructor
    IDLCompiler();
    /// open the application
    virtual bool Open();
    /// close the application
    virtual void Close();
    /// run the application, return when user wants to exit
    virtual void Run();
    /// get error string
    const Util::String& GetError() const;
    /// get error line number
    int GetErrorLineNumber() const;

private:
    /// set error string
    void SetError(const Util::String& err);
    /// parse source file into C++ object hierarchy
    bool ParseFile(const IO::URI& uri);
    /// compile a single file
    bool CompileFile(const IO::URI& fileUri);

	Util::String outputDir;
    Util::Array<IO::URI> fileUris;
    Util::String error;
    int errorLineNumber;
    Ptr<IDLDocument> document;
};

//------------------------------------------------------------------------------
/**
*/
inline void
IDLCompiler::SetError(const Util::String& err)
{
    this->error = err;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLCompiler::GetError() const
{
    return this->error;
}

//------------------------------------------------------------------------------
/**
*/
inline int
IDLCompiler::GetErrorLineNumber() const
{
    return this->errorLineNumber;
}

} // namespace Tools
//------------------------------------------------------------------------------
#endif

