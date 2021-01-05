//------------------------------------------------------------------------------
//  util/commandlineargs.cc
//  (C) 2005 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "util/commandlineargs.h"

namespace Util
{
#if !__OSX__
using namespace Math;
#endif
    
//------------------------------------------------------------------------------
/**
*/
CommandLineArgs::CommandLineArgs()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
CommandLineArgs::CommandLineArgs(const String& l)
{
    this->args = l.Tokenize(" \t\n=", '"');
}

//------------------------------------------------------------------------------
/**
*/
CommandLineArgs::CommandLineArgs(int argc, const char** argv)
{
    int i;
    for (i = 0; i < argc; i++)
    {
        this->args.Append(argv[i]);
    }
}

//------------------------------------------------------------------------------
/**
    Put out a missing argument error.
*/
void
CommandLineArgs::Error(const String& argName) const
{
    n_error("CmdLine: arg '%s' not found!", argName.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
    Returns the command name.
*/
const String&
CommandLineArgs::GetCmdName() const
{
    n_assert(this->args.Size() > 0);
    return this->args[0];
}

//------------------------------------------------------------------------------
/**
    Returns true if argument exists.
*/
bool
CommandLineArgs::HasArg(const String& name, const IndexT start) const
{
    IndexT index = this->args.FindIndex(name, start);
    return (index != InvalidIndex);
}

//------------------------------------------------------------------------------
/**
    Returns index of argument. Throws an error if argument not found.
*/
int
CommandLineArgs::FindArgIndex(const String& name, const IndexT start) const
{
    n_assert(name.IsValid());
    IndexT i = this->args.FindIndex(name, start);
    if (InvalidIndex == i)
    {
        this->Error(name);
    }
    return i;
}

//------------------------------------------------------------------------------
/**
*/
const String&
CommandLineArgs::GetString(const String& name, const String& defaultValue) const
{
    if (this->HasArg(name))
    {
        return this->args[this->FindArgIndex(name) + 1];
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Util::String>
CommandLineArgs::GetStrings(const String& name, const String& defaultValue) const
{
	Util::Array<Util::String> ret;
	if (this->HasArg(name))
	{
		IndexT start = 0;
		IndexT index = this->FindArgIndex(name, start) + 1;
		while (index != InvalidIndex)
		{
			ret.Append(this->args[index]);
			start = index;
			if (!this->HasArg(name, start))
				break;
			index = this->FindArgIndex(name, start) + 1;
		}
	}
	else
	{
		ret.Append(defaultValue);
	}
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
int
CommandLineArgs::GetInt(const String& name, int defaultValue) const
{
    if (this->HasArg(name))
    {
        return this->args[this->FindArgIndex(name) + 1].AsInt();
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<int>
CommandLineArgs::GetInts(const String& name, int defaultValue) const
{
	Util::Array<int> ret;
	if (this->HasArg(name))
	{
		IndexT start = 0;
		IndexT index = this->FindArgIndex(name, start) + 1;
		while (index != InvalidIndex)
		{
			ret.Append(this->args[index].AsInt());
			start = index;
			if (!this->HasArg(name, start))
				break;
			index = this->FindArgIndex(name, start) + 1;
		}
	}
	else
	{
		ret.Append(defaultValue);
	}
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
float
CommandLineArgs::GetFloat(const String& name, float defaultValue) const
{
    if (this->HasArg(name))
    {
        return this->args[this->FindArgIndex(name) + 1].AsFloat();
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<float>
CommandLineArgs::GetFloats(const String& name, float defaultValue) const
{
	Util::Array<float> ret;
	if (this->HasArg(name))
	{
		IndexT start = 0;
		IndexT index = this->FindArgIndex(name, start) + 1;
		while (index != InvalidIndex)
		{
			ret.Append(this->args[index].AsFloat());
			start = index;
			if (!this->HasArg(name, start))
				break;
			index = this->FindArgIndex(name, start) + 1;
		}
	}
	else
	{
		ret.Append(defaultValue);
	}
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
bool
CommandLineArgs::GetBool(const String& name, bool defaultValue) const
{
    if (this->HasArg(name))
    {
        return this->args[this->FindArgIndex(name) + 1].AsBool();
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<bool>
CommandLineArgs::GetBools(const String& name, bool defaultValue) const
{
	Util::Array<bool> ret;
	if (this->HasArg(name))
	{
		IndexT start = 0;
		IndexT index = this->FindArgIndex(name, start) + 1;
		while (index != InvalidIndex)
		{
			ret.Append(this->args[index].AsBool());
			start = index;
			if (!this->HasArg(name, start))
				break;
			index = this->FindArgIndex(name, start) + 1;
		}
	}
	else
	{
		ret.Append(defaultValue);
	}
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
bool
CommandLineArgs::GetBoolFlag(const String& name) const
{
    return this->HasArg(name);
}

#if !__OSX__
//------------------------------------------------------------------------------
/**
*/
vec4
CommandLineArgs::GetVec4(const String& name, const vec4& defaultValue) const
{
    if (this->HasArg(name))
    {
        return this->args[this->FindArgIndex(name) + 1].AsVec4();
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Math::vec4>
CommandLineArgs::GetVec4s(const String& name, const Math::vec4& defaultValue) const
{
	Util::Array<Math::vec4> ret;
	if (this->HasArg(name))
	{
		IndexT start = 0;
		IndexT index = this->FindArgIndex(name, start) + 1;
		while (index != InvalidIndex)
		{
			ret.Append(this->args[index].AsVec4());
			start = index;
			if (!this->HasArg(name, start))
				break;
			index = this->FindArgIndex(name, start) + 1;
		}
	}
	else
	{
		ret.Append(defaultValue);
	}
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
mat4
CommandLineArgs::GetMat4(const String& name, const mat4& defaultValue) const
{
    if (this->HasArg(name))
    {
        return this->args[this->FindArgIndex(name) + 1].AsMat4();
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Math::mat4>
CommandLineArgs::GetMat4s(const String& name, const Math::mat4& defaultValue) const
{
	Util::Array<Math::mat4> ret;
	if (this->HasArg(name))
	{
		IndexT start = 0;
		IndexT index = this->FindArgIndex(name, start) + 1;
		while (index != InvalidIndex)
		{
			ret.Append(this->args[index].AsMat4());
			start = index;
			if (!this->HasArg(name, start))
				break;
			index = this->FindArgIndex(name, start) + 1;
		}
	}
	else
	{
		ret.Append(defaultValue);
	}
	return ret;
}
#endif
    
//------------------------------------------------------------------------------
/**
*/
SizeT
CommandLineArgs::GetNumArgs() const
{
    n_assert(this->args.Size() > 0);
    return this->args.Size() - 1;
}

//------------------------------------------------------------------------------
/**
*/
const String&
CommandLineArgs::GetStringAtIndex(IndexT index) const
{
    return this->args[index + 1];
}

//------------------------------------------------------------------------------
/**
*/
int
CommandLineArgs::GetIntAtIndex(IndexT index) const
{
    return this->args[index + 1].AsInt();
}

//------------------------------------------------------------------------------
/**
*/
float
CommandLineArgs::GetFloatAtIndex(IndexT index) const
{
    return this->args[index + 1].AsFloat();
}

//------------------------------------------------------------------------------
/**
*/
bool
CommandLineArgs::GetBoolAtIndex(IndexT index) const
{
    return this->args[index + 1].AsBool();
}

#if !__OSX__    
//------------------------------------------------------------------------------
/**
*/
vec4
CommandLineArgs::GetVec4AtIndex(IndexT index) const
{
    return this->args[index + 1].AsVec4();
}

//------------------------------------------------------------------------------
/**
*/
mat4
CommandLineArgs::GetMat4AtIndex(IndexT index) const
{
    return this->args[index + 1].AsMat4();
}
#endif
    
//------------------------------------------------------------------------------
/**
*/
void 
CommandLineArgs::AppendCommandString(const CommandLineArgs& otherArgs)
{
    IndexT i;
    for (i = 0; i < otherArgs.args.Size(); ++i)
    {
        this->args.Append(otherArgs.args[i]);
    }
}
} // namespace Util
