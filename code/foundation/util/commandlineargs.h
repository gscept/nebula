#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::CommandLineArgs
    
    A universal cmd line argument parser. The command line string
    must have the form 

    cmd arg0[=]value0 arg1[=]value1 arg2[=]value2
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "util/array.h"
#include "util/string.h"
#include "math/vec4.h"
#include "math/mat4.h"

//------------------------------------------------------------------------------
namespace Util
{
class CommandLineArgs
{
public:
    /// default constructor
    CommandLineArgs();
    /// construct from Win32-style command string
    CommandLineArgs(const String& cmdLine);
    /// construct from posix style command string
    CommandLineArgs(int argc, const char** argv);
    /// get the command name
    const String& GetCmdName() const;
    /// return true if arg exists
    bool HasArg(const String& arg, const IndexT start = 0) const;
    /// get string value
    const String& GetString(const String& name, const String& defaultValue = "") const;
    /// get all string values
    Util::Array<Util::String> GetStrings(const String& name, const String& defaultValue = "") const;
    /// get int value
    int GetInt(const String& name, int defaultValue = 0) const;
    /// get all int values
    Util::Array<int> GetInts(const String& name, int defaultValue = 0) const;
    /// get float value
    float GetFloat(const String& name, float defaultValue = 0.0f) const;
    /// get all float values
    Util::Array<float> GetFloats(const String& name, float defaultValue = 0.0f) const;
    /// get bool value (key=value)
    bool GetBool(const String& name, bool defaultValue=false) const;
    /// get all bool values
    Util::Array<bool> GetBools(const String& name, bool defaultValue = false) const;
    /// get bool flag (args only needs to exist to trigger as true)
    bool GetBoolFlag(const String& name) const;
    #if !__OSX__
    /// get vec4 value 
    Math::vec4 GetVec4(const String& name, const Math::vec4& defaultValue = Math::vec4()) const;
    /// get all vec4 values
    Util::Array<Math::vec4> GetVec4s(const String& name, const Math::vec4& defaultValue = Math::vec4()) const;
    /// get mat4 value
    Math::mat4 GetMat4(const String& name, const Math::mat4& defaultValue = Math::mat4()) const;
    /// get all mat4 values
    Util::Array<Math::mat4> GetMat4s(const String& name, const Math::mat4& defaultValue = Math::mat4()) const;
    #endif
    
    /// returns true if the argument list is empty
    bool IsEmpty() const;
    /// get number of arguments (exluding command name)
    SizeT GetNumArgs() const;
    /// get string argument at index
    const String& GetStringAtIndex(IndexT index) const;
    /// get int value at index
    int GetIntAtIndex(IndexT index) const;
    /// get float value at index
    float GetFloatAtIndex(IndexT index) const;
    /// get bool value at index
    bool GetBoolAtIndex(IndexT index) const;
    #if !__OSX__
    /// get vec4 value at index
    Math::vec4 GetVec4AtIndex(IndexT index) const;
    /// get mat4 value at index
    Math::mat4 GetMat4AtIndex(IndexT index) const;
    #endif
    /// append other command string
    void AppendCommandString(const CommandLineArgs& otherArgs);
    /// get any key value pairs from args
    const Dictionary<String, String>& GetPairs() const;

private:
    /// put out an error
    void Error(const String& argName) const;
    /// find index of argument, fails hard with error msg if not found
    int FindArgIndex(const String& argName, const IndexT start = 0) const;
    /// sort out any explicit key=value arguments into separate structure
    void ParseIntoGroups(const Array<String>& Tokenized);

    Array<String> args;
    Dictionary<String,String> keyPairs;
};


//------------------------------------------------------------------------------
/**
*/
inline bool 
CommandLineArgs::IsEmpty() const
{
    return this->args.Size() == 0;
}

//------------------------------------------------------------------------------
/**
*/
inline const Dictionary<String, String>& 
CommandLineArgs::GetPairs() const
{
    return this->keyPairs;
}

} // namespace Util
//------------------------------------------------------------------------------
