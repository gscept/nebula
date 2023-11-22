//------------------------------------------------------------------------------
//  logger.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "logger.h"
#include "text.h"

namespace ToolkitUtil
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
Logger::Logger() :
    verbose(true)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Logger::~Logger()
{
    // empty
}


//------------------------------------------------------------------------------
/**
*/
void
Logger::Error(const char* msg, ...)
{
    va_list argList;
    va_start(argList, msg);
    String str;
    str.FormatArgList(msg, argList);
    this->messages.Append(String("[ERROR] ") + str);
    if (this->verbose)
    {
        n_error("%s%s", this->indent.AsCharPtr(), Text(this->messages.Back()).Color(TextColor::Red).AsCharPtr());
    }
    va_end(argList);
}
     
//------------------------------------------------------------------------------
/**
*/
void
Logger::Warning(const char* msg, ...)
{
    va_list argList;
    va_start(argList, msg);
    String str;
    str.FormatArgList(msg, argList);
    this->messages.Append(String("[WARNING] ") + str);
    if (this->verbose)
    {
        n_warning("%s%s", this->indent.AsCharPtr(), Text(this->messages.Back()).Color(TextColor::Yellow).AsCharPtr());
    }
    va_end(argList);
}

//------------------------------------------------------------------------------
/**
*/
void
Logger::Print(const char* msg, ...)
{
    va_list argList;
    va_start(argList, msg);
    String str;
    str.FormatArgList(msg, argList);
    this->messages.Append(str);
    if (this->verbose)
    {
        n_printf("%s%s", this->indent.AsCharPtr(), str.AsCharPtr());
    }
    va_end(argList);
}

} // namespace ToolkitUtil
