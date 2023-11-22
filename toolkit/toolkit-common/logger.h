#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::Logger
    
    A simple logger for error messages and warnings.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "util/string.h"
#include "util/array.h"
#include "util/stack.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class Logger
{
public:
    /// constructor
    Logger();
    /// destructor
    virtual ~Logger();
    
    /// set verbosity flag (display messages on stdout, default is true)
    void SetVerbose(bool b);
    /// get verbosity flag
    bool GetVerbose() const;
    /// put a formatted error message
    virtual void Error(const char* msg, ...);
    /// put a formatted warning message
    virtual void Warning(const char* msg, ...);
    /// put a formatted message
    virtual void Print(const char* msg, ...);

    /// Indent logger
    void Indent();
    /// Unindent logger
    void Unindent();

protected:
    bool verbose;

    Util::String indent;
    Util::Array<Util::String> messages;
};

//------------------------------------------------------------------------------
/**
*/
inline void
Logger::SetVerbose(bool b)
{
    this->verbose = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Logger::GetVerbose() const
{
    return this->verbose;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
Logger::Indent()
{
    this->indent += "    ";
}

//------------------------------------------------------------------------------
/**
*/
inline void 
Logger::Unindent()
{
    this->indent = this->indent.ExtractRange(0, this->indent.Length() - 4);
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------


    