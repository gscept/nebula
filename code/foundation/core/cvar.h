#pragma once
//------------------------------------------------------------------------------
/**
    @file cvar.h

    Contains API for creating, writing to, and reading from a Core::CVar

    @struct Core::CVar

    A console variable.
    Always handle as a opaque object. Pass the CVar* to the various functions to
    perform operations on the variable.

    @copyright
    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
namespace Core
{

/// Forward declaration of a Core::CVar
struct CVar;

/// Denotes the type of a Core::CVar
enum CVarType
{
    CVar_Int,
    CVar_Float,
    CVar_String
};

/// Used to create a Core::CVar
struct CVarCreateInfo
{
    const char* name;
    const char* defaultValue;
    CVarType type;
};

/// Create a console variable
CVar* CVarCreate(CVarCreateInfo const&);
/// Get a console variable
CVar* CVarGet(const char* name);
/// Parse value from c string and assign to cvar
void CVarParseWrite(CVar*, const char* value);
/// Write float value to cvar
void CVarWriteFloat(CVar*, float value);
/// Write int value to cvar
void CVarWriteInt(CVar*, int value);
/// Write string value to cvar
void CVarWriteString(CVar*, const char* value);
/// Read int value from cvar
int const CVarReadInt(CVar*);
/// Read float value from cvar
float const CVarReadFloat(CVar*);
/// Read string value from cvar
const char* CVarReadString(CVar*);

} // namespace Core